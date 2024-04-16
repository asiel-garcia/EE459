#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "adcph.h"

int main(){
    DDRC &= ~(1<<PC2);//PH sensor
    PORTC |= (1<<PC2);

    adc_init_ph();

    return 0;
}

int ph(void){
    char buf[10];
    char temp;
    uint32_t avgValue;
    char buffer1[20];
    char buffer2[20];

    for(int i=0;i<10;i++){ 
        buf[i]=adc_read_ph();
        delay(10);
    }
    for(int i=0;i<9;i++){
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
    avgValue=0;
    for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];

    sprintf(buffer1, "Raw: %lu", avgValue);
    lcd_moveto(0,0);
    lcd_stringout(buffer1); // Display raw avgval ph value on LCD

    float phValue=(int)avgValue*5.0/1024/6; //convert the analog into millivolt
    phValue=3.5*phValue;                      //convert the millivolt into pH value
    
    
    sprintf(buffer2, "pH:  %f", phValue);
    lcd_moveto(1,0);
    lcd_stringout(buffer2); //Display ph

    return avgValue;
}