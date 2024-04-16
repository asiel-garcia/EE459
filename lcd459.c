#include <avr/io.h>
#include <util/delay.h>

#include "lcd459.h"

#define FOSC 7372800		// Clock frequency
#define BAUD 9600              // Baud rate used by the LCD
#define MYUBRR FOSC/16/BAUD-1   // Value for UBRR0 register

/*
  lcd_init - Initialize the LCD
*/
void lcd_init()
{
    _delay_ms(250);             // Wait 500msec for the LCD to start up
    _delay_ms(250);
    sci_out(0xfe);              // Clear the screen
    sci_out(0x51);
}

/*
  moveto - Move the cursor to the row and column given by the arguments.
  Row is 0 or 1, column is 0 - 15.
*/
void lcd_moveto(unsigned char row, unsigned char col)
{
    sci_out(0xfe);              // Set the cursor position
    sci_out(0x45);
    if(row==0){
      sci_out(col);
    }
    else if (row==2){
      sci_out(0x14+col);
    }
    else if (row==1){
      sci_out(0x40+col);
    }
    else if (row==3){
      sci_out(0x54+col);
    }
}


/*
  lcd_stringout - Print the contents of the character string "str"
  at the current cursor position.
*/
void lcd_stringout(char *str)
{
    sci_outs(str);              // Output the string
}

/* ----------------------------------------------------------------------- */

/*
  sci_init - Initialize the SCI port
*/
void sci_init(void) {
  UBRR0 = MYUBRR;          // Set baud rate
  UCSR0B |= (1 << TXEN0);  // Turn on transmitter
  UCSR0C = (3 << UCSZ00);  // Set for asynchronous operation, no parity, 
                            // one stop bit, 8 data bits
}

/*
  sci_out - Output a byte to SCI port
*/
void sci_out(char ch)
{
    while ( (UCSR0A & (1<<UDRE0)) == 0);
    UDR0 = ch;
}

/*
  sci_outs - Print the contents of the character string "s" out the SCI
  port. The string must be terminated by a zero byte.
*/
void sci_outs(char *s)
{
    char ch;

    while ((ch = *s++) != '\0')
        sci_out(ch);
}