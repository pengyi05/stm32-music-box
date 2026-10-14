#include <REGX52.H>
unsigned char pos=0;
void LED_Init(){P2=0xFF;}
void LED_Flow()
{
    P2=~(1<<pos);
    pos=(pos+1)%8;
}
