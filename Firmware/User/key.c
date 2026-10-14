#include <REGX52.H>
#include "delay.h"
#include "music.h"
sbit KEY1=P3^0;
sbit KEY2=P3^1;
sbit KEY3=P3^2;

void Key_Scan(void)
{
    if(KEY2==0){DelayMs(10);while(KEY2==0); if(song_id>0)song_id--;}
    if(KEY3==0){DelayMs(10);while(KEY3==0); song_id++;}
}
