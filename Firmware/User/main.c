/*
 * ============================================================================
 *  AT89C52 音乐盒 —— 主程序
 * ============================================================================
 *
 *  【项目概述】
 *  基于 AT89C52（8051 架构，11.0592MHz 晶振）的音频播放系统。
 *  支持三首曲目播放、上一个/下一个/暂停/单曲循环切换，
 *  并以 8x8 点阵显示当前曲目编号、以 7 个 LED 指示当前音高。
 *
 *  【硬件连接】
 *  P0  —— 8x8 点阵列控制（需外接上拉电阻，8051 的 P0 为开漏输出）
 *  P1   bit0 —— 蜂鸣器（无源蜂鸣器，固定频率由定时器0产生）
 *  P1   bit1~7 —— 7 个 LED，分别对应 L5S~L7/M1~M7 七个音高
 *  P2  —— 8x8 点阵行控制
 *  P3.1 —— 播放/暂停按键
 *  P3.2 —— 下一曲按键
 *  P3.3 —— 上一曲按键
 *  P3.7 —— 单曲循环开关（低电平开启单曲循环，高电平为列表循环）
 *
 *  【定时器分配】
 *  Timer0（模式1，16 位）—— 控制蜂鸣器翻转频率，即音高。
 *                          中断服务程序每次翻转一次引脚，故为方波半周期。
 *  Timer1（模式1，16 位）—— 控制音符时值。定时 20ms 中断一次，
 *                          每次中断将剩余时间减 20，即实现毫秒级计时。
 *
 *  【关键时序计算】
 *  8051 一个机器周期 = 12 个振荡周期：
 *      机器周期 = 12 / 11.0592MHz ≈ 1.085us
 *
 *  音高计算：音符常量存的是"到下次翻转的计数值"（65536 - 装载值）。
 *      例如 L6 = 0xFB90，即计数 65536 - 64400 = 1136 次翻转间隔；
 *      半周期 = 1136 × 1.085us ≈ 1232us，
 *      完整方波周期 = 2 × 1232us ≈ 2465us，对应频率 ≈ 406Hz。
 *
 *  时值计算：Timer1 每次中断间隔 = (65536 - 0xB800) × 1.085us
 *      = 18432 × 1.085us ≈ 20ms（整除，无误差）
 *      故剩余时间以毫秒计，每 20ms 递减 20。
 *
 *  节拍计算：F = 60000 / BPM，即一个四分音符的毫秒数。
 *      BPM=120 时 F=500ms（四分音符），F/2 为八分音符，F/4 为十六分音符。
 *
 *  【内存空间管理（8051 特有）】
 *  本程序大量使用 code 关键字，这是 Keil C51 的扩展：
 *      code   —— 数据存放在程序 Flash（8KB），不占用 RAM
 *      xdata  —— 数据存放在片外扩展 RAM
 *      data   —— 数据存放在片内 RAM（128B），访问速度最快
 *
 *  AT89C52 内部 RAM 仅 128 字节，而音频乐谱表有数百字节，
 *  因此所有常量表（乐谱、LED 字模、跳转表）必须用 code 修饰，
 *  否则 RAM 溢出，程序行为将不可预知。这是 8051 开发最常见的坑之一。
 *
 * ============================================================================
 */

#include <reg51.h>

/* ===========================================================================
 * 一、乐理与节拍定义
 * =========================================================================*/

/*
 * BPM —— 每分钟节拍数（Beats Per Minute）。
 * 决定音乐的整体快慢，可通过修改此值实现变速播放而不改变音调。
 */
#define BPM 120

/*
 * F —— 一个四分音符的持续时间，单位毫秒。
 * 计算：60 秒/分钟 × 1000 毫秒/秒 ÷ BPM = 60000 / BPM
 * BPM=120 时 F=500，因此：
 *      F    = 四分音符（一拍）
 *      F/2  = 八分音符（半拍）
 *      F/4  = 十六分音符（1/4 拍）
 */
#define F (60000/BPM)

/*
 * 音符频率常量
 * 定义的是定时器重装载值 65536 - 周期计数值。
 * 由于 8051 的定时器是"向上计数到 65536 溢出"，因此装载值越小，计数越多，
 * 翻转越慢，音高越低。
 */
#define N   0x0000    /* 休止符（不发声，用于乐谱中的停顿）      */
#define L1  0xFC89    /* 低音 L1                                  */
#define L2  0xFCFA
#define L3  0xFDB3
#define L4  0xFE28
#define L5  0xFEF4
#define L5S 0xFB4C    /* L5 的升调版本，用于特定旋律段落          */
#define L6  0xFB90
#define L7  0xFC0C
#define M1  0xFC44    /* 中音 M1                                  */
#define M2  0xFCAD
#define M3  0xFD0A
#define M4  0xFD34
#define M5  0xFD82
#define M6  0xFDC8
#define M7  0xFE06

/* ===========================================================================
 * 二、8x8 点阵显示定义
 * =========================================================================*/

/* 行选端口与列选端口 */
#define RowPort P2     /* 行控制：P2 输出行扫描信号 */
#define ColPort P0     /* 列控制：P0 输出列扫描信号（需外接上拉电阻） */

/*
 * 音符数据结构 —— 用"数据结构描述乐谱"是本项目的核心设计思路。
 *
 * 设计理由：
 *   若把旋律写成 if-else 或 switch-case 的播放流程，代码会随曲目数量
 *   指数级膨胀，且难以维护。改用数据驱动后，新增曲目只需追加一个数组，
 *   无需修改任何播放逻辑代码 —— 这与"用表驱动代替逻辑分支"的思想一致，
 *   在协议解析、状态机设计中同样适用。
 *
 * 字段说明：
 *   half_period —— 定时器0 的周期计数值，决定音高（半周期翻转为完整方波）
 *   duration    —— 该音符的持续毫秒数，由定时器1 递减计时
 */
typedef struct {
    unsigned int half_period;
    unsigned int duration;
} gequ;

/*
 * LED 数字字模（点阵显示用）
 * 每个字节代表一行 8 个像素，位序为高到低。
 * 例如 0x3E = 0011 1110，对应底部 5 个点亮的横线。
 */
unsigned char code LED_Num1[8] = {   /* 显示 "1" */
    0x08, 0x0C, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x3E
};

unsigned char code LED_Num2[8] = {   /* 显示 "2" */
    0x1E, 0x10, 0x10, 0x1E,
    0x02, 0x02, 0x02, 0x1E
};

unsigned char code LED_Num3[8] = {   /* 显示 "3" */
    0x1E, 0x10, 0x10, 0x1C,
    0x10, 0x10, 0x10, 0x1E
};

/* ===========================================================================
 * 三、乐谱数据
 * ===========================================================================
 * 乐谱以"音符表"形式存储，每一项为 {音高, 时值}。
 * 表尾以 {0, 0} 作为结束标志 —— 因为 0x0000 被定义为休止符 N，
 * 同时充当了数组终止哨兵，省去了额外的长度字段。
 */

/* 曲目一：Bad Apple（高节奏快歌，用于测试密集时值） */
gequ code bad_apple[] = {
    {L6,F/2},{L7,F/2},{M1,F/2},{M2,F/2},{M3,F},{M6,F/2},{M5,F/2},
    {M3,F},{L6,F},{M3,F/2},{M2,F/2},{M1,F/2},{L7,F/2},
    {L6,F/2},{L7,F/2},{M1,F/2},{M2,F/2},{M3,F},{M2,F/2},{M1,F/2},
    {L7,F/2},{L6,F/2},{L7,F/2},{M1,F/2},{L7,F/2},{L6,F/2},{L5S,F/2},{L7,F/2},
    {L6,F/2},{L7,F/2},{M1,F/2},{M2,F/2},{M3,F},{M6,F/2},{M5,F/2},
    {M3,F},{L6,F},{M3,F/2},{M2,F/2},{M1,F/2},{L7,F/2},
    {L6,F/2},{L7,F/2},{M1,F/2},{M2,F/2},{M3,F},{M2,F/2},{M1,F/2},
    {L7,F},{M1,F},{M2,F},{M3,F},
    {L6,F/2},{L7,F/2},{M1,F/2},{M2,F/2},{M3,F},{M6,F/2},{M5,F/2},
    {M3,F},{L6,F},{M3,F/2},{M2,F/2},{M1,F/2},{L7,F/2},
    {L6,F/2},{L7,F/2},{M1,F/2},{M2,F/2},{M3,F},{M2,F/2},{M1,F/2},
    {L7,F/2},{L6,F/2},{L7,F/2},{M1,F/2},{L7,F/2},{L6,F/2},{L5S,F/2},{L7,F/2},
    {L6,F/2},{L7,F/2},{M1,F/2},{M2,F/2},{M3,F},{M6,F/2},{M5,F/2},
    {M3,F},{L6,F},{M3,F/2},{M2,F/2},{M1,F/2},{L7,F/2},
    {L6,F/2},{L7,F/2},{M1,F/2},{M2,F/2},{M3,F},{M2,F/2},{M1,F/2},
    {L7,F},{M1,F},{M2,F},{M3,F},
    {0,0}   /* 曲终标志 */
};

/* 曲目二：See You Again（含 F/4 十六分音符，用于测试最短时值精度） */
gequ code see_you_again[] = {
    {M1,F}, {M3,F}, {M6,F}, {M1,F},{M1,F},{M1,F/4}, {M2,F/4},{M3,F/4},
    {M2,F/4}, {M1,F/4}, {M2,F/4},
    {M1,F}, {M3,F}, {M6,F}, {M1,F},{M1,F},{M1,F/4}, {M2,F/4},{M3,F/4},
    {M2,F/4}, {M1,F/4}, {M2,F/4},
    {M1,F}, {M3,F}, {M6,F}, {M1,F},{M1,F},{M1,F/4}, {M2,F/4},{M3,F/4},
    {M2,F/4}, {M1,F/4}, {M2,F/4},
    {M1,F}, {M3,F}, {M6,F}, {M1,F},{M1,F},
    {M1,F},{M3,F}, {M5,F},{M6,F},{M6,F}, {M5,F},{M6,F},
    {M1,F}, {M2,F}, {M2,F},{M1,F}, {M2,F},{M3,F}, {M3,F},
    {M3,F/2},{M5,F/2},{M6,F},{M7,F},{M6,F},{M5,F},{M3,F},{M2,F},{M2,F},{M1,F},{M2,F},{M2,F},{M2,F/2},{M1,F},{M1,F},
    {M1,F/2}, {M3,F/2}, {M5,F/2}, {M6,F},{M6,F},{M5,F},{M6,F},{M6,F},
    {M1,F},{M2,F},{M2,F},{M1,F},{M3,F}, {M3,F},
    {M2,F/4},{M3,F/4},{M5,F/2},{M6,F},{M1,F},{M2,F},{M3,F},{M2,F},{M1,F},{M5,F/4},{M6,F/4},{M1,F/2},{M2,F},{M3,F},{M2,F},{M1,F},{M1,F},
    {M5,F/4},{M6,F/4},{M1,F/2},{M2,F},{M3,F},{M2,F},{M1,F},{M1,F},
    {0,0}   /* 曲终标志 */
};

/* 曲目三：两只老虎（经典儿歌，慢节奏，用于测试长时值表现） */
gequ code liangzhilaohu[] = {
    {M1,F/2},{M2,F/2},{M3,F/2},{M1,F/2},{M3,F/2},{M2,F/2},{M1,F/2},{M2,F/2},
    {M3,F},{M1,F},{M3,F/2},{M2,F/2},{M1,F/2},{M2,F/2},
    {L5,F},{L5,F},{M1,F},{M1,F},{M2,F/2},{M3,F/2},
    {M2,F/2},{M1,F/2},{L5,F},{L5,F},{M1,F},{M1,F},
    {M2,F/2},{M3,F/2},{M2,F/2},{M1,F/2},
    {0,0}   /* 曲终标志 */
};

/* ===========================================================================
 * 四、硬件引脚定义
 * =========================================================================*/

sbit SPEAKER = P1^0;      /* 无源蜂鸣器，固定频率由定时器0驱动 */
sbit KEY_PLAY = P3^1;     /* 播放/暂停按键 */
sbit KEY_NEXT = P3^2;     /* 下一曲按键 */
sbit KEY_PREV = P3^3;     /* 上一曲按键 */
sbit SW_LOOP  = P3^7;     /* 单曲循环开关：0=单曲循环，1=列表循环 */

/* ===========================================================================
 * 五、全局变量
 * =========================================================================*/

/* 播放状态 */
unsigned char note_index;       /* 当前音符在乐谱数组中的下标         */
unsigned int  current_period;   /* 当前音符的定时器周期计数值（音高） */
unsigned int  remain_time;      /* 当前音符剩余时间，单位毫秒         */
bit           is_playing;       /* 播放标志：1=播放中，0=暂停          */

/* 切歌标志与流程控制 */
bit next_flag;                  /* 下一曲请求标志                     */
bit prev_flag;                  /* 上一曲请求标志                     */
bit need_break;                 /* 打断标志：用于在按键响应时退出音符等待 */

/* 曲目管理 */
gequ code *current_music;                  /* 当前曲目指针，指向 code 空间   */
unsigned char   current_song = 0;          /* 当前曲目序号                   */
gequ code * const song_list[] = {           /* 曲目跳转表（存于 code 空间）  */
    bad_apple,
    see_you_again,
    liangzhilaohu
};
unsigned char song_count;                  /* 曲目总数                       */
unsigned char display_num = 1;             /* 点阵显示的曲目编号（1~3）      */

/* 单曲循环开关状态 */
bit single_loop_enable = 0;

/* ===========================================================================
 * 六、函数声明
 * =========================================================================*/

void delay(unsigned int t);
void Delay_ms(unsigned int ms);
void stop_sound(void);
void play_note(unsigned int period, unsigned int duration);
void Display_Num(unsigned char num);
void check_play_pause(void);
void check_next(void);
void check_prev(void);
void check_loop_switch(void);
void switch_song(unsigned char idx);
void update_led_by_note(unsigned int period);
void turn_off_leds(void);
void init_leds(void);

/* ===========================================================================
 * 七、音高指示 LED 控制
 * =========================================================================*/

/*
 * 功能：根据当前音高点亮对应的 LED（共 7 个，覆盖 L5S~M7 七个音区）。
 *
 * 硬件连接：P1 的 bit1~bit7 分别接 7 个 LED，P1.0 已被蜂鸣器占用，
 *          因此操作时必须保留 bit0 的状态，不能直接整体赋值 P1。
 */
void update_led_by_note(unsigned int period) {
    unsigned char led_mask = 0x00;

    switch(period) {
        case L5S:                    /* L5 升调，最低音区 */
            led_mask |= (1 << 1);
            break;
        case L6: case L7: case M1:   /* 低音区 */
            led_mask |= (1 << 2);
            break;
        case L1: case M2: case L2:
            led_mask |= (1 << 3);
            break;
        case M3: case M4: case M5:   /* 中音区 */
            led_mask |= (1 << 4);
            break;
        case L3: case M6:
            led_mask |= (1 << 5);
            break;
        case M7: case L4:
            led_mask |= (1 << 6);
            break;
        case L5:
            led_mask |= (1 << 7);
            break;
        default:                     /* 休止符，熄灭全部 LED */
            led_mask = 0x00;
            break;
    }

    /*
     * 位运算技巧：(P1 & 0x01) 保留蜂鸣器引脚状态，
     * 再与 led_mask 按位或，实现"只改 LED 不动蜂鸣器"。
     */
    P1 = (P1 & 0x01) | led_mask;
}

/* 熄灭全部音高指示 LED（保留蜂鸣器引脚） */
void turn_off_leds(void) {
    P1 &= 0x01;
}

/*
 * 上电自检：依次点亮 7 个 LED 验证硬件连线是否正常。
 * 先全亮再全灭，形成"跑马灯"效果，便于肉眼确认焊接质量。
 */
void init_leds(void) {
    unsigned char i;
    P1 &= 0x01;                  /* 全灭 */
    P1 |= 0xFE;                  /* 全亮（bit1~7） */
    for(i = 0; i < 200; i++) Delay_ms(1);

    P1 &= 0x01;                  /* 全灭 */
    for(i = 0; i < 200; i++) Delay_ms(1);

    for(i = 1; i <= 7; i++) {    /* 逐个点亮 */
        P1 = (P1 & 0x01) | (1 << i);
        Delay_ms(100);
    }
    P1 &= 0x01;                  /* 最后全灭 */
}

/* ===========================================================================
 * 八、延时函数
 * =========================================================================*/

/*
 * 毫秒级延时。
 *
 * 注意：这是"空循环延时"，不精确，实际时间取决于编译器优化、
 * 晶振频率和指令周期。本项目的时值控制并不依赖它 —— 时值由 Timer1
 * 中断精确计时，因此本函数仅用于 LED 刷新和按键消抖等非时序敏感场景。
 */
void Delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i = ms; i > 0; i--)
        for(j = 110; j > 0; j--);
}

/*
 * 极短延时，用于按键消抖后的二次确认。
 * 8051 执行一条空循环约需 1~2 个机器周期（1~2.2us），
 * delay(1000) 约产生 1~2ms 的延迟。
 */
void delay(unsigned int t) {
    unsigned int i;
    for(i = 0; i < t; i++);
}

/* ===========================================================================
 * 九、8x8 点阵显示
 * =========================================================================*/

/*
 * 动态扫描显示指定数字（1~3）。
 *
 * 原理：利用视觉暂留效应，快速逐行扫描。
 * 每轮流程：
 *   1) 列端置全 1（关闭所有列）
 *   2) 行端输出该行的点模数据
 *   3) 列端拉低一位，选通当前显示位
 *   4) 延时 2ms（保证人眼可分辨）
 * 8 行循环一遍即完成一帧，帧刷新需 >= 8×2 = 16ms，故 60Hz 帧率无闪烁。
 */
void Display_Num(unsigned char num) {
    unsigned char i;
    unsigned char *p;         /* 指向当前要显示的字模 */

    switch(num) {
        case 1: p = LED_Num1; break;
        case 2: p = LED_Num2; break;
        case 3: p = LED_Num3; break;
        default: p = LED_Num1; break;
    }

    for(i = 0; i < 8; i++) {
        ColPort = 0xFF;              /* 先关闭列选 */
        RowPort = p[i];              /* 输出本行点模 */
        ColPort = ~(1 << i);         /* 拉低当前列，选通该行 */
        Delay_ms(2);                 /* 显示保持 */
    }
}

/* ===========================================================================
 * 十、声音控制
 * =========================================================================*/

/* 停止发声：关闭定时器0并复位蜂鸣器引脚、熄灭音高 LED */
void stop_sound(void) {
    TR0 = 0;               /* 关闭定时器0 计数 */
    ET0 = 0;               /* 关闭定时器0 中断允许 */
    SPEAKER = 0;           /* 蜂鸣器静默（输出低电平） */
    turn_off_leds();       /* 熄灭音高指示 LED */
}

/*
 * 启动一个音符。
 *
 * 参数 period   —— 定时器0 的 16 位周期计数值，决定音高
 * 参数 duration —— 该音符持续的毫秒数，写入 remain_time 供 Timer1 递减
 *
 * 实现要点：
 *   8051 定时器0 工作在模式1（16 位），需分两次写入 TH0 和 TL0，
 *   且必须先写高字节再写低字节；若在赋值过程中计数器溢出，
 *   可能导致首次中断时机偏差（虽然本项目影响不大，但是通用注意事项）。
 */
void play_note(unsigned int period, unsigned int duration) {
    if(period == 0) {
        /* 休止符：不发声，仅停止当前音 */
        stop_sound();
        return;
    }

    current_period = period;
    remain_time    = duration;

    TH0 = period >> 8;     /* 高字节 */
    TL0 = period & 0xFF;   /* 低字节 */

    ET0 = 1;               /* 允许定时器0 中断 */
    TR0 = 1;               /* 启动定时器0 */
}

/*
 * 定时器0 中断服务程序 —— 负责音高（频率）生成。
 *
 * 每进入一次中断就翻转蜂鸣器引脚，因此两次中断构成一个完整方波周期。
 * 中断周期 = (65536 - 装载值) × 机器周期 ≈ 半周期。
 *
 * 关键设计：中断程序必须"短小精悍"，只做翻转和重装载两件事。
 * 若在此加入显示刷新或按键扫描，会显著拉长中断周期导致音调失真。
 */
void t0_isr(void) interrupt 1 {
    SPEAKER = ~SPEAKER;            /* 翻转输出，形成方波 */
    TH0 = current_period >> 8;     /* 重新装载，保证周期稳定 */
    TL0 = current_period & 0xFF;
}

/*
 * 定时器1 中断服务程序 —— 负责时值（时长）计时。
 *
 * 重装载值 0xB800（十进制 47104）对应中断间隔：
 *     (65536 - 47104) × 1.085us = 18432 × 1.085us ≈ 20.0ms
 *
 * 故每次中断代表经过 20ms，将 remain_time 减去 20 即可实现毫秒级计时。
 * 这种"定时器中断累减"的计时方式，比在主循环里用 Delay 计数更精确，
 * 因为它不受主循环执行时间影响。
 */
void t1_isr(void) interrupt 3 {
    TH1 = 0xB8;                     /* 重装载，维持 20ms 定时间隔 */
    TL1 = 0x00;

    if(remain_time > 0) {
        if(remain_time >= 20)
            remain_time -= 20;      /* 正常递减 */
        else
            remain_time = 0;        /* 最后不足 20ms 的余数直接归零 */
    }
}

/* ===========================================================================
 * 十一、按键与开关检测
 * ===========================================================================
 * 所有按键均采用"检测按下 -> 延时消抖 -> 二次确认 -> 等待释放"的经典流程。
 * 消抖是必须的：机械按键在按下/释放瞬间会产生几十毫秒的抖动，
 * 若不处理会被识别为多次连续点击。
 */

/* 播放 / 暂停切换 */
void check_play_pause(void) {
    if(KEY_PLAY == 0) {             /* 检测到按下（低电平有效） */
        delay(1000);                /* 延时消抖 */
        if(KEY_PLAY == 0) {         /* 二次确认，排除抖动误判 */
            is_playing = !is_playing;

            if(!is_playing) {
                stop_sound();       /* 暂停时关闭定时器 */
            }
            while(!KEY_PLAY);       /* 等待按键释放，防止连击 */
        }
    }
}

/* 下一曲 */
void check_next(void) {
    if(KEY_NEXT == 0) {
        delay(1000);
        if(KEY_NEXT == 0) {
            next_flag  = 1;         /* 置位请求标志 */
            need_break = 1;         /* 打断当前音符等待 */
            while(!KEY_NEXT);       /* 等待释放 */
        }
    }
}

/* 上一曲 */
void check_prev(void) {
    if(KEY_PREV == 0) {
        delay(1000);
        if(KEY_PREV == 0) {
            prev_flag  = 1;
            need_break = 1;
            while(!KEY_PREV);
        }
    }
}

/*
 * 单曲循环开关检测（P3.7）
 *
 *   P3.7 = 0（接地）-> 开启单曲循环：当前曲目播完后从头再播一遍
 *   P3.7 = 1（悬空）-> 列表循环    ：当前曲目播完后自动切到下一曲
 *
 * 该功能在曲目播放结束时的分支处生效。
 */
void check_loop_switch(void) {
    if(SW_LOOP == 0) {
        single_loop_enable = 1;
    } else {
        single_loop_enable = 0;
    }
}

/* ===========================================================================
 * 十二、曲目切换
 * =========================================================================*/

/*
 * 切换到指定序号的曲目。
 * 参数 idx —— 目标曲目序号（0 ~ song_count-1）
 *
 * 重置内容：停止发声 -> 更新曲目指针 -> 音符下标归零 -> 置播放状态。
 * 采用"指针 + 下标"的方式遍历乐谱，切歌时无需搬运数组数据，O(1) 复杂度。
 */
void switch_song(unsigned char idx) {
    stop_sound();

    current_song = idx;
    current_music = song_list[current_song];   /* 取新曲目的首地址 */
    note_index = 0;                            /* 从头开始播放       */
    is_playing  = 1;
    need_break  = 0;
    display_num = current_song + 1;            /* 显示编号从 1 开始  */
}

/* ===========================================================================
 * 十三、主函数
 * =========================================================================*/

void main(void) {
    unsigned char i;

    /*
     * 曲目总数计算。
     * sizeof(song_list) 得到指针数组总字节数（3 个指针 × 2 字节 = 6），
     * sizeof(song_list[0]) 得到单个指针大小（8051 下为 2 字节），
     * 两者相除即为曲目数。用这种方式而非硬编码 3，扩展新曲目时无需修改。
     */
    song_count = sizeof(song_list) / sizeof(song_list[0]);

    /* ---- 定时器与中断初始化 ---- */
    TMOD = 0x11;        /* GATE1=0, T1=方式1(16位), GATE0=0, T0=方式1(16位) */
    TH0  = 0;
    TL0  = 0;
    TH1  = 0xB8;        /* Timer1 定时 20ms，用于时值计数 */
    TL1  = 0x00;
    ET0  = 0;           /* 先关闭定时器0 中断，待播放时由 play_note 使能 */
    ET1  = 1;           /* 允许定时器1 中断（时值计时常开） */
    TR1  = 1;           /* 启动定时器1 */
    EA   = 1;           /* 全局中断允许 */

    /* ---- 播放状态初始化 ---- */
    current_music = song_list[0];  /* 默认载入第一首曲目 */
    is_playing    = 0;             /* 上电处于暂停状态 */
    note_index    = 0;
    next_flag     = 0;
    prev_flag     = 0;
    need_break    = 0;
    display_num   = current_song + 1;
    single_loop_enable = 0;

    /* ---- 端口初始化 ---- */
    P1 = 0x00;         /* P1 全低：关闭全部 LED，蜂鸣器静默 */
    SW_LOOP = 1;       /* 置高 P3.7（准双向口写 1 相当于释放），默认列表循环 */

    /* ---- LED 硬件自检 ---- */
    init_leds();

    /* ================= 主循环 ================= */
    while(1) {

        /* 刷新曲目编号显示 */
        Display_Num(display_num);

        /* 扫描按键与循环开关 */
        check_play_pause();
        check_next();
        check_prev();
        check_loop_switch();

        /* ---- 处理"下一曲"请求 ---- */
        if(next_flag) {
            next_flag = 0;
            i = current_song + 1;
            if(i >= song_count) i = 0;        /* 循环回到第一首 */
            switch_song(i);
            continue;                          /* 跳过本次音符播放 */
        }

        /* ---- 处理"上一曲"请求 ---- */
        if(prev_flag) {
            prev_flag = 0;
            if(current_song == 0)
                i = song_count - 1;            /* 第一首的上一首是最后一首 */
            else
                i = current_song - 1;
            switch_song(i);
            continue;
        }

        /* 暂停状态下空转，只处理按键 */
        if(!is_playing) continue;

        /* ---- 读取当前音符 ---- */
        current_period = current_music[note_index].half_period;

        /*
         * 遇到 {0,0} 表示曲目结束。
         * 此时根据单曲循环开关决定行为：
         *   开启单曲循环 -> 下标归零，重播当前曲目
         *   列表循环     -> 置下一曲标志，自动切歌
         */
        if(current_period == 0) {
            if(single_loop_enable) {
                note_index = 0;
                continue;
            } else {
                next_flag = 1;
                continue;
            }
        }

        /* 按音高点亮对应 LED */
        update_led_by_note(current_period);

        /* 启动该音符（设置定时器0 频率与剩余时间） */
        play_note(current_period, current_music[note_index].duration);

        /*
         * 等待本音符播放结束。
         *
         * 这里是一个"忙等待"循环，但设计上有两点讲究：
         *  1) 循环体内不停刷新点阵显示，避免显示闪烁；
         *  2) 循环体内不停扫描按键，保证切歌/暂停能即时响应。
         *
         * need_break 用于在用户按下切歌/暂停时立即退出等待，
         * 实现"打断"语义；退出后重新判断 is_playing 决定是继续
         * 播放当前音符（暂停后恢复）还是彻底停止。
         */
        while(remain_time > 0 && !need_break) {
            Display_Num(display_num);
            check_play_pause();
            check_next();
            check_prev();
            check_loop_switch();

            if(!is_playing) {
                stop_sound();

                /* 暂停期间空转，仅保持显示与按键响应 */
                while(!is_playing && !need_break) {
                    Display_Num(display_num);
                    check_play_pause();
                    check_next();
                    check_prev();
                    check_loop_switch();
                }

                /* 若恢复播放且未被打断，则从剩余时间继续播放当前音符 */
                if(is_playing && !need_break) {
                    update_led_by_note(current_period);
                    play_note(current_period, remain_time);
                }
            }
        }

        /* 本音符结束，停声并熄灭音高 LED */
        stop_sound();

        /* 因切歌被打断 -> 下标归零，等待主循环处理切歌 */
        if(need_break) {
            need_break = 0;
            continue;
        }

        /* 正常播完 -> 音符下标前进到下一个 */
        note_index++;
    }
}