#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "lcd459.h"

unsigned char new_state, old_state;
volatile unsigned char changed = 0;  // Flag for state change
int count = 0;		// Count to display
unsigned char a, b;

ISR(PCINT2_vect)
{
	//checks for  bit change
	if((PIND & (1 << 2)) != 0 && (PIND & (1 << 4)) != 0)
	{
		a = 1;
		b = 1;
	}
	else if((PIND & (1 << 2)) != 0 && (PIND & (1 << 4)) == 0)
	{
		a = 1;
		b = 0;
	}
	else if((PIND & (1 << 2)) == 0 && (PIND & (1 << 4)) == 0)
	{
		a = 0;
		b = 0;
	}
	else if((PIND & (1 << 2)) == 0 && (PIND & (1 << 4)) != 0)
	{
		a = 0;
		b = 1;
	}

	//changes states based on bit change
	if (old_state == 0) {

	    // Handle A and B inputs for state 0
		if(a == 1) //a is 1
		{
			new_state = 1;
			count++;
		}
		else if(b == 1) //b is 1
		{
			new_state = 2;
			count--;
		}

	}
	else if (old_state == 1) {

	    // Handle A and B inputs for state 1
		if(b == 1) //b is 1
		{
			new_state = 3;
			count++;
		}
		else if(a == 0) //b is 0
		{
			new_state = 0;
			count--;
		}

	}
	else if (old_state == 3) {

	 // Handle A and B inputs for state 2
		if(a == 0) //a is 0
		{
			new_state = 2;
			count++;
		}
		else if(b == 0) //b  is 0
		{
			new_state = 1;
			count--;
		}

	}
	else {   // old_state = 2
		// Handle A and B inputs for state 3
		if(b == 0) //b is 0
		{
			new_state = 0;
			count++;
		}
		else if(a == 1) //a is 1
		{
			new_state = 3;
			count--;
		}
	}

	if(new_state != old_state) //if a state change happens flag is raised
	{
		changed = 1;
		old_state = new_state;
	}
}

int main(void) {

  PCICR |= (1 << PCIE2); //initializes isr stuff
	PCMSK2 |= ((1 << PCINT18) | (1 << PCINT20));
	sei();

  // Initialize DDR and PORT registers and LCD
	DDRD &= ~((1 << 2) | (1 << 4)); //for rotary encoder
	PORTD |= ((1 << 2) | (1 << 4));

  sci_init(); // Initialize the SCI port
  lcd_init(); // Initialize the LCD

	lcd_moveto(0,0);
	count = 0;
	char buf1[5];
	snprintf(buf1, 5, "%4d", count);
	lcd_stringout(buf1);

  if((PIND & (1 << 2)) == 0 && (PIND & (1 << 4)) == 0)
	{
		a = 1;
		b = 1;
	}
	else if((PIND & (1 << 2)) != 0 && (PIND & (1 << 4)) == 0)
	{
		a = 0;
		b = 1;
	}
	else if((PIND & (1 << 2)) == 0 && (PIND & (1 << 4)) != 0)
	{
		a = 1;
		b = 0;
	}
	else if((PIND & (1 << 2)) != 0 && (PIND & (1 << 4)) != 0)
	{
		a = 0;
		b = 0;
	}

	//decides initial state
  if (!b && !a)
	old_state = 0;
  else if (!b && a)
	old_state = 1;
  else if (b && !a)
	old_state = 2;
  else
	old_state = 3;

  new_state = old_state;

  while (1) {                 // Loop forever
    if (changed == 1)  //if a change in the bits happened then run this bc then states have changed
    {
      lcd_moveto(0,0); //screen updater
      snprintf(buf1, 5, "%4d", count);
      lcd_stringout(buf1);

      if (changed)
      {
        changed = 0;        // Reset changed flag
      }
    }
  }
}
