#include <REGX52.H>
#include "delay.h"
unsigned char song_id=0;
sbit BEEP=P1^0;

unsigned int code tone[]={0,262,294,330,349,392,440,494,523};
unsigned char code song1[]={1,1,5,5,6,6,5,4,4,3,3,2,2,1};
unsigned char idx=0;

void beep(unsigned int f)
{
    unsigned int i,j;
    if(f==0){DelayMs(150);return;}
    for(i=0;i<120;i++)
    {
        for(j=0;j<500/f;j++);
        BEEP=~BEEP;
    }
}

void Play_Music(void)
{
    beep(tone[song1[idx]]);
    idx++;
    if(idx>=sizeof(song1)) idx=0;
}
