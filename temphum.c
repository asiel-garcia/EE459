#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <math.h>

#include "lcd459.h"
#include "i2c.h"

#define FOSC 7372800		// Clock frequency
#define BDIV (FOSC / 100000 - 16) / 2 + 1 // Value for Temp/Hum sensor running at 100kHz (prescalar = 1)

void check_temp_hum(void);


int main(void) {
    
    PORTC |= (1<<PC5) | (1<<PC4); //initialize for i2c

    sci_init(); // Initialize the SCI port
    lcd_init(); // Initialize the LCD
    i2c_init(BDIV); //initialize i2c

    
    while (1) {// Loop forever
      sci_out(0xfe);              // Clear the screen
      sci_out(0x51);
      check_temp_hum();


      _delay_ms(1000); // Wait for 1 second
    }
    
    

    return 0;   /* never reached */
}

void check_temp_hum(void){
  unsigned char temp_hum_status;
  unsigned char wbuf[] = {0x24, 0x0B}; //write buff, start measurement command is 0x240B
  unsigned char rbuf[6]; //data buff
  char buffer1[20];
  char buffer2[20];
  char buffer3[20];
  uint32_t relative_hum, farenheit;

  temp_hum_status = i2c_io(0x88, wbuf, 2, rbuf, 6); //sensor address 0x88
  sprintf(buffer3, "Status: %d", temp_hum_status); //print status
  lcd_moveto(3,0);
  lcd_stringout(buffer3);

  unsigned int temp = (rbuf[0] << 8) | rbuf[1]; //populate 32 bit unsigned int w 2 bytes
  unsigned int hum = (rbuf[3] << 8) | rbuf[4]; //populate 32 bit unsigned int w 2 bytes

  farenheit = -49 + ((315 * ((temp) / 655.35))/100); //convert raw temp to farenheit
  relative_hum = (hum / 655.35); //convert raw hum to relative hum

  sprintf(buffer1, "Temp: %lu", farenheit);
  sprintf(buffer2, "Hum:  %lu", relative_hum);

  lcd_moveto(0,0);
  lcd_stringout(buffer1); // Display temp value on LCD
  lcd_moveto(1,0);
  lcd_stringout(buffer2); //Display humidity
}