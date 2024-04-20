#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "lcd459.h"
#include "adcph.h"

void read_ph(void){
    char bufph[20];
    //char bufraw[20];
    uint32_t avgValuetemp;
    uint16_t buf[10], temp;


    for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
    { 
        buf[i]=adc_read_ph();
        _delay_ms(10);
    }
    for(int i=0;i<9;i++)        //sort the analog from small to large
    {
        for(int j=i+1;j<10;j++)
        {
            if(buf[i]>buf[j])
            {
                temp=buf[i];
                buf[i]=buf[j];
                buf[j]=temp;
            }
        }
    }

    avgValuetemp=0;
    for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    {
        avgValuetemp+=buf[i];
    }

    uint16_t avgValue = avgValuetemp / 6; //avg adc value
    unsigned int milliv = avgValue * 5 / 10.24; //convert the analog into millivolt using 10.24 instead of 1024 to preserve decimal
    unsigned int tempph = 23 + (3.5 * milliv); // holding value = ph * 100 (+adjustment)
    unsigned int phValue = tempph / 100; //getting truncated ph value

    unsigned int decimalVal = tempph % 100;

    /*
    sprintf(bufraw, "Raw: %u", avgValue);
    lcd_moveto(0,0);
    lcd_stringout(bufph);
    */

    sprintf(bufph, "pH: %u.%02u", phValue, decimalVal);
    lcd_moveto(3,0);
    lcd_stringout(bufph);
}

