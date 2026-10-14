#include <reg51.h>

#define BPM 120
#define F (60000/BPM)

// ??
#define N 0x0000
#define L1 0xFC89
#define L2 0xFCFA
#define L3 0xFDB3
#define L4 0xFE28
#define L5 0xFEF4
#define L6 0xFB90
#define L7 0xFC0C

// ??
#define M1 0xFC44
#define M2 0xFCAD
#define M3 0xFD0A
#define M4 0xFD34
#define M5 0xFD82
#define M6 0xFDC8
#define M7 0xFE06
#define L5S 0xFB4C

/******************************************
 * 8x8 LED ??????
 ******************************************/
#define RowPort P2
#define ColPort P0

typedef struct {
    unsigned int half_period;
    unsigned int duration;
} gequ;

/******************************************
 * ?? "1" ???
 ******************************************/
unsigned char code LED_Num1[8] = {
    0x08, 0x0C, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x3E
};

/******************************************
 * ?? "2" ???
 ******************************************/
unsigned char code LED_Num2[8] = {
    0x1E, 0x10, 0x10, 0x1E,
    0x02, 0x02, 0x02, 0x1E
};

/******************************************
 * ?? "3" ???
 ******************************************/
unsigned char code LED_Num3[8] = {
    0x1E, 0x10, 0x10, 0x1C,
    0x10, 0x10, 0x10, 0x1E
};

/******************************************
 * ???? - Bad Apple
 ******************************************/
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
    {0,0}
};

/******************************************
 * ???? - See You Again
 ******************************************/
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
    {0,0}
};

/******************************************
 * ???? - ????
 ******************************************/
gequ code liangzhilaohu[] = {
    {M1,F/2},{M2,F/2},{M3,F/2},{M1,F/2},{M3,F/2},{M2,F/2},{M1,F/2},{M2,F/2},
    {M3,F},{M1,F},{M3,F/2},{M2,F/2},{M1,F/2},{M2,F/2},
    {L5,F},{L5,F},{M1,F},{M1,F},{M2,F/2},{M3,F/2},
    {M2,F/2},{M1,F/2},{L5,F},{L5,F},{M1,F},{M1,F},
    {M2,F/2},{M3,F/2},{M2,F/2},{M1,F/2},
    {0,0}
};

/******************************************
 * ??????
 ******************************************/
sbit SPEAKER = P1^0;
sbit KEY_PLAY = P3^1;
sbit KEY_NEXT = P3^2;
sbit KEY_PREV = P3^3;
sbit SW_LOOP = P3^7;      // ??:??????(???=????)

/******************************************
 * ????
 ******************************************/
unsigned char note_index;
unsigned int current_period;
unsigned int remain_time;
bit is_playing;
bit next_flag, prev_flag;
bit need_break;
gequ code *current_music;
unsigned char current_song = 0;
gequ code * const song_list[] = {bad_apple, see_you_again, liangzhilaohu};
unsigned char song_count;
unsigned char display_num = 1;

// ??:??????(?P3.7??)
bit single_loop_enable = 0;

/******************************************
 * ????
 ******************************************/
void delay(unsigned int t);
void Delay_ms(unsigned int ms);
void stop_sound(void);
void play_note(unsigned int period, unsigned int duration);
void Display_Num(unsigned char num);
void check_play_pause(void);
void check_next(void);
void check_prev(void);
void check_loop_switch(void);  // ??:????????
void switch_song(unsigned char idx);

/******************************************
 * LED????(?????)
 ******************************************/
void update_led_by_note(unsigned int period) {
    unsigned char led_mask = 0x00;
    
    switch(period) {
        case L5S: led_mask |= (1 << 1); break;
        case L6: case L7: case M1: led_mask |= (1 << 2); break;
        case L1: case M2: case L2: led_mask |= (1 << 3); break;
        case M3: case M4: case M5: led_mask |= (1 << 4); break;
        case L3: case M6: led_mask |= (1 << 5); break;
        case M7: case L4: led_mask |= (1 << 6); break;
        case L5: led_mask |= (1 << 7); break;
        default: led_mask = 0x00; break;
    }
    
    P1 = (P1 & 0x01) | led_mask;
}

void turn_off_leds(void) {
    P1 &= 0x01;
}

void init_leds(void) {
    unsigned char i;
    P1 &= 0x01;
    P1 |= 0xFE;
    for(i = 0; i < 200; i++) Delay_ms(1);
    P1 &= 0x01;
    for(i = 0; i < 200; i++) Delay_ms(1);
    for(i = 1; i <= 7; i++) {
        P1 = (P1 & 0x01) | (1 << i);
        Delay_ms(100);
    }
    P1 &= 0x01;
}

/******************************************
 * ????
 ******************************************/
void Delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i = ms; i > 0; i--)
        for(j = 110; j > 0; j--);
}

void delay(unsigned int t) {
    unsigned int i;
    for(i = 0; i < t; i++);
}

/******************************************
 * 8x8 LED??????
 ******************************************/
void Display_Num(unsigned char num) {
    unsigned char i;
    unsigned char *p;
    
    switch(num) {
        case 1: p = LED_Num1; break;
        case 2: p = LED_Num2; break;
        case 3: p = LED_Num3; break;
        default: p = LED_Num1; break;
    }
    
    for(i = 0; i < 8; i++) {
        ColPort = 0xFF;
        RowPort = p[i];
        ColPort = ~(1 << i);
        Delay_ms(2);
    }
}

/******************************************
 * ??????
 ******************************************/
void stop_sound(void) {
    TR0 = 0;
    ET0 = 0;
    SPEAKER = 0;
    turn_off_leds();
}

void play_note(unsigned int period, unsigned int duration) {
    if(period == 0) {
        stop_sound();
        return;
    }
    current_period = period;
    remain_time = duration;
    TH0 = period >> 8;
    TL0 = period & 0xFF;
    ET0 = 1;
    TR0 = 1;
}

void t0_isr(void) interrupt 1 {
    SPEAKER = ~SPEAKER;
    TH0 = current_period >> 8;
    TL0 = current_period & 0xFF;
}

void t1_isr(void) interrupt 3 {
    TH1 = 0xB8;
    TL1 = 0x00;
    if(remain_time > 0) {
        if(remain_time >= 20) remain_time -= 20;
        else remain_time = 0;
    }
}

/******************************************
 * ??????
 ******************************************/
void check_play_pause(void) {
    if(KEY_PLAY == 0) {
        delay(1000);
        if(KEY_PLAY == 0) {
            is_playing = !is_playing;
            if(!is_playing) {
                stop_sound();
            }
            while(!KEY_PLAY);
        }
    }
}

void check_next(void) {
    if(KEY_NEXT == 0) {
        delay(1000);
        if(KEY_NEXT == 0) {
            unsigned char next_song;
            next_flag = 1;
            need_break = 1;
            next_song = current_song + 1;
            if(next_song >= song_count) next_song = 0;
            display_num = next_song + 1;
            while(!KEY_NEXT);
        }
    }
}

void check_prev(void) {
    if(KEY_PREV == 0) {
        delay(1000);
        if(KEY_PREV == 0) {
            unsigned char prev_song;
            prev_flag = 1;
            need_break = 1;
            if(current_song == 0) prev_song = song_count - 1;
            else prev_song = current_song - 1;
            display_num = prev_song + 1;
            while(!KEY_PREV);
        }
    }
}

/******************************************
 * ??:????????
 * P3.7 = 0(???)???????
 * P3.7 = 1(???)???????
 ******************************************/
void check_loop_switch(void) {
    // ??P3.7??
    // ???(??)-> ??????
    // ???(??)-> ??????
    if(SW_LOOP == 0) {
        single_loop_enable = 1;
    } else {
        single_loop_enable = 0;
    }
}

/******************************************
 * ????
 ******************************************/
void switch_song(unsigned char idx) {
    stop_sound();
    current_song = idx;
    current_music = song_list[current_song];
    note_index = 0;
    is_playing = 1;
    need_break = 0;
    display_num = current_song + 1;
}

/******************************************
 * ???
 ******************************************/
void main(void) {
    unsigned char i;
    song_count = sizeof(song_list) / sizeof(song_list[0]);
    
    // ??????
    TMOD = 0x11;
    TH0 = 0;
    TL0 = 0;
    TH1 = 0xB8;
    TL1 = 0x00;
    ET0 = 0;
    ET1 = 1;
    TR1 = 1;
    EA = 1;
    
    // ?????
    current_music = song_list[0];
    is_playing = 0;
    note_index = 0;
    next_flag = 0;
    prev_flag = 0;
    need_break = 0;
    display_num = current_song + 1;
    single_loop_enable = 0;
    
    // ???P1??P3.7
    P1 = 0x00;
    SW_LOOP = 1;  // ??P3.7?????(??1)
    
    // LED????
    init_leds();
    
    while(1) {
        // ????????????
        Display_Num(display_num);
        
        // ???????
        check_play_pause();
        check_next();
        check_prev();
        check_loop_switch();  // ??:??????????
        
        // ?????(????)
        if(next_flag) {
            next_flag = 0;
            // ??:????????,???????,????????
            i = current_song + 1;
            if(i >= song_count) i = 0;
            switch_song(i);
            continue;
        }
        
        // ?????
        if(prev_flag) {
            prev_flag = 0;
            if(current_song == 0) i = song_count - 1;
            else i = current_song - 1;
            switch_song(i);
            continue;
        }
        
        // ?????????
        if(!is_playing) continue;
        
        // ??????
        current_period = current_music[note_index].half_period;
        
        // ??????
        if(current_period == 0) {
            if(single_loop_enable) {
                // ??????:????,????????
                note_index = 0;
                continue;
            } else {
                // ????:??????
                next_flag = 1;
                continue;
            }
        }
        
        // ??????????LED
        update_led_by_note(current_period);
        
        // ??????
        play_note(current_period, current_music[note_index].duration);
        
        // ????????
        while(remain_time > 0 && !need_break) {
            Display_Num(display_num);
            check_play_pause();
            check_next();
            check_prev();
            check_loop_switch();  // ????????????
            if(!is_playing) {
                stop_sound();
                while(!is_playing && !need_break) {
                    Display_Num(display_num);
                    check_play_pause();
                    check_next();
                    check_prev();
                    check_loop_switch();
                }
                if(is_playing && !need_break) {
                    update_led_by_note(current_period);
                    play_note(current_period, remain_time);
                }
            }
        }
        
        // ???????LED
        stop_sound();
        
        if(need_break) {
            need_break = 0;
            continue;
        }
        
        note_index++;
    }
}
