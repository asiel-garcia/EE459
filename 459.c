#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/atomic.h>

#include "lcd459.h"
#include "i2c.h"
#include "adc459.h"
#include "temphum.h"
#include "adcph.h"
#include "phmeter.h"


#define FOSC 7372800
#define BDIV (FOSC / 100000 - 16) / 2 + 1
#define DEBOUNCE_TIME 500 // milliseconds


typedef enum {
    DEFAULT_VIEW,
    MENU_VIEW,
    EDIT_PH_LEVEL,
    EDIT_NUTRIENT_LEVEL,
    EDIT_WATER_LEVEL
} SystemState;

volatile SystemState state = DEFAULT_VIEW;
volatile int menuIndex = 0;
volatile int menuCounter = 0;
volatile float pHLevel = 6.5;
volatile int nutrientLevel = 2;
volatile int desiredWaterLevel = 3;
volatile int currentWaterLevel = 0;
volatile unsigned long lastInteraction;
volatile int MENU_ITEMS = 4;
volatile unsigned long milliseconds = 0;
volatile unsigned long lastDebounceTime = 0;



void sci_init(void);
void lcd_init(void);
void rotary_init(void);
void adc_init(void);
void check_temp_hum(void);
void updateDisplay(void);
void updateSelector(void);
void read_water_levels(void);
void control_water_pump(void);
void read_ph(void);

uint8_t i2c_io(uint8_t, uint8_t *, uint16_t, uint8_t *, uint16_t);
void i2c_init(uint8_t);

ISR(TIMER0_COMPA_vect) {
    milliseconds++;
}

unsigned long millis() {
    unsigned long millis_return;

    // Ensure this cannot be disrupted
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        millis_return = milliseconds;
    }

    return millis_return;
}

void timer0_init() {
    // Set up Timer0
    // Assuming prescaler is set to 64 and F_CPU is 7372800
    TCCR0A = (1 << WGM01); // Configure timer for CTC mode
    TCCR0B = (1 << CS01) | (1 << CS00); // Set prescaler to 64
    OCR0A = 114; // Set CTC compare value to 1ms at 7.3728MHz AVR clock, with a prescaler of 64
    TIMSK0 = (1 << OCIE0A); // Enable CTC interrupt

    sei(); // Enable global interrupts
}

ISR(INT0_vect) {
    static uint8_t lastA = 0;
    static uint8_t lastB = 0;
    uint8_t currentA = (PIND & (1 << PD2)) != 0;
    uint8_t currentB = (PIND & (1 << PD4)) != 0;  // Assuming B is on PD4

    if (currentA != lastA) {  // Only act on changes of A
        if (currentA == currentB) {
            // Counter-clockwise rotation
            if (state == MENU_VIEW) {
                menuCounter--;
                if(menuCounter<-10){
                	menuIndex--;
                	menuCounter=0;
             		}
                if (menuIndex < 0) menuIndex = MENU_ITEMS - 1;  // Wrap around
                updateSelector();
            } else if (state == EDIT_PH_LEVEL) {
                pHLevel -= 0.1;
                if (pHLevel < 3.0) pHLevel = 3.0;  // Clamp the pH level
           		  updateDisplay();  // Refresh the LCD based on the current state

            } else if (state == EDIT_NUTRIENT_LEVEL) {
                nutrientLevel--;
                if (nutrientLevel < 0) nutrientLevel = 0;  // Clamp the nutrient level
                updateDisplay();  // Refresh the LCD based on the current state

            } else if (state == EDIT_WATER_LEVEL) {
                desiredWaterLevel--;
                if (desiredWaterLevel < 1) desiredWaterLevel = 1;  // Clamp the water level setting
                updateDisplay();  // Refresh the LCD based on the current state

            }
        } else {
            // Clockwise rotation
            if (state == MENU_VIEW) {
                menuCounter++;
                if(menuCounter>10){
                	menuIndex++;
                	menuCounter=0;
             		}
                if (menuIndex >= MENU_ITEMS) menuIndex = 0;  // Wrap around
                updateSelector();
            } else if (state == EDIT_PH_LEVEL) {
                pHLevel += 0.1;
                if (pHLevel > 10.0) pHLevel = 10.0;  // Clamp the pH level
                updateDisplay();  // Refresh the LCD based on the current state

            } else if (state == EDIT_NUTRIENT_LEVEL) {
                nutrientLevel++;
                if (nutrientLevel > 20) nutrientLevel = 20;  // Clamp the nutrient level
                updateDisplay();  // Refresh the LCD based on the current state

            } else if (state == EDIT_WATER_LEVEL) {
                desiredWaterLevel++;
                if (desiredWaterLevel > 3) desiredWaterLevel = 3;  // Clamp the water level setting
                updateDisplay();  // Refresh the LCD based on the current state

            }
      		}
    }
    else if (currentB != lastB) {  // Only act on changes of A
        if (currentA == currentB) {
            // Clockwise rotation
            if (state == MENU_VIEW) {
                menuCounter++;
                if(menuCounter>10){
                	menuIndex++;
                	menuCounter=0;
             		}
                if (menuIndex >= MENU_ITEMS) menuIndex = 0;  // Wrap around
                updateSelector();
            } else if (state == EDIT_PH_LEVEL) {
                pHLevel += 0.1;
                if (pHLevel > 10.0) pHLevel = 10.0;  // Clamp the pH level
                updateDisplay();  // Refresh the LCD based on the current state

            } else if (state == EDIT_NUTRIENT_LEVEL) {
                nutrientLevel++;
                if (nutrientLevel > 20) nutrientLevel = 20;  // Clamp the nutrient level
                updateDisplay();  // Refresh the LCD based on the current state

            } else if (state == EDIT_WATER_LEVEL) {
                desiredWaterLevel++;
                if (desiredWaterLevel > 3) desiredWaterLevel = 3;  // Clamp the water level setting
                updateDisplay();  // Refresh the LCD based on the current state
            }
        } else {
            // Counter-clockwise rotation
            if (state == MENU_VIEW) {
                menuCounter--;
                if(menuCounter<-10){
                	menuIndex--;
                	menuCounter=0;
             		}
                if (menuIndex < 0) menuIndex = MENU_ITEMS - 1;  // Wrap around
                updateSelector();
            } else if (state == EDIT_PH_LEVEL) {
                pHLevel -= 0.1;
                if (pHLevel < 3.0) pHLevel = 3.0;  // Clamp the pH level
           		  updateDisplay();  // Refresh the LCD based on the current state

            } else if (state == EDIT_NUTRIENT_LEVEL) {
                nutrientLevel--;
                if (nutrientLevel < 0) nutrientLevel = 0;  // Clamp the nutrient level
                updateDisplay();  // Refresh the LCD based on the current state

            } else if (state == EDIT_WATER_LEVEL) {
                desiredWaterLevel--;
                if (desiredWaterLevel < 1) desiredWaterLevel = 1;  // Clamp the water level setting
                updateDisplay();  // Refresh the LCD based on the current state
            }
   				}
   			}
    lastA = currentA;
    lastB = currentB;
}

ISR(INT1_vect) {
    // ISR for pushbutton to select menu items or confirm settings
    //lastInteraction = millis();
	 // unsigned long currentTime = millis(); // Function to get the current time in ms
   //  if ((currentTime - lastDebounceTime) > DEBOUNCE_TIME) {
   //      lastDebounceTime = currentTime; // Update the last debounce time
	// 			}
		_delay_ms(100);
    switch (state) {
        case DEFAULT_VIEW:
        _delay_ms(5);
            state = MENU_VIEW;
            break;
        case MENU_VIEW:
         _delay_ms(5);
            if (menuIndex == 0) {
                _delay_ms(5);
                state = EDIT_PH_LEVEL;
                _delay_ms(100);
            } else if (menuIndex == 1) {
                _delay_ms(5);
                state = EDIT_NUTRIENT_LEVEL;
                _delay_ms(100);
            } else if (menuIndex == 2) {
                _delay_ms(5);
                state = EDIT_WATER_LEVEL;
                _delay_ms(100);
            } else if (menuIndex == 3) {  // Handling the "Go back" selection
                _delay_ms(5);
                state = DEFAULT_VIEW;
                _delay_ms(100);
            }
            break;
        case EDIT_PH_LEVEL:
        _delay_ms(5);
        case EDIT_NUTRIENT_LEVEL:
        _delay_ms(5);
        case EDIT_WATER_LEVEL:
        _delay_ms(5);
            state = MENU_VIEW;
            control_water_pump(); // Check and control water pump after settings
            break;
    }
    updateDisplay();
}

void setup() {
    sci_init();
    lcd_init();
    i2c_init(BDIV);
    rotary_init();
    adc_init();
    adc_init_ph();
    PORTC |= (1<<PC5) | (1<<PC4); //initialize for i2c
    timer0_init(); // Set up Timer0 for millis() function

    DDRC &= ~(1 << 1); //pH sensor
    PORTC |= (1 << 1); //pH sensor


    //DDRB |= (1 << PB7); // Configure PB7 as output for relay
    sei();  // Enable global interrupts
    //lastInteraction = millis();
}

void loop() {
    if (state == DEFAULT_VIEW) {
        check_temp_hum();
        read_water_levels();
        _delay_ms(1000);
        /*if (millis() - lastInteraction > 20000) {  // No interaction for 20s
            state = DEFAULT_VIEW;
            updateDisplay();
        }*/
    }
    _delay_ms(500);  // Main loop delay
}

int main(void) {
    setup();
    //pump set ups
    DDRB |= (1 << 7) | (1 << 0);
	DDRD |= (1 << 5) | (1 << 6) | (1 << 7);

	PORTB &= ~((1 << 7) | (1 << 0));
	PORTD &= ~((1 << 5) | (1 << 6) | (1 << 7));

    while (1) {
        loop();
        PORTB |= (1<<7); // run pump to plants without turning off
        if((pHLevel - phValue) > 0.3){
            //add acid
            PORTD |= (1<<7);
            _delay_ms(1000);
            PORTD &= ~(1<<7);
            _delay_ms(500);
        }
        else if((pHLevel - phValue) < 0.3){
            //add base
            PORTB |= (1<<0);
            _delay_ms(1000);
            PORTB &= ~(1<<0);
            _delay_ms(500);
        }

        PORTD |= (1<<5);
        for(int i = 0; i < nutrientLevel; i++){
            _delay_ms(1000);
        }
		PORTD &= ~(1<<5);
		_delay_ms(500);

		PORTD |= (1<<6);
		for(int i = 0; i < nutrientLevel; i++){
            _delay_ms(1000);
        }
		PORTD &= ~(1<<6);
		_delay_ms(500);
        
    }
    return 0;
}

void updateSelector() {
	if (state == MENU_VIEW ) {
		  // Indicate selected item
		 lcd_moveto(menuIndex, 19);
     lcd_stringout("<");
     if (menuIndex!=0){
     lcd_moveto(0, 19);
     lcd_stringout(" ");
   		}
     if (menuIndex!=1){
     lcd_moveto(1, 19);
     lcd_stringout(" ");
   		}
   	 if (menuIndex!=2){
     lcd_moveto(2, 19);
     lcd_stringout(" ");
   		}
   	 if (menuIndex!=3){
     lcd_moveto(3, 19);
     lcd_stringout(" ");
   		}
	}
}

void updateDisplay() {
    char buffer[20];
    switch (state) {
        case DEFAULT_VIEW:
        		sci_out(0xfe);              // Clear the screen
      			sci_out(0x51);
            lcd_moveto(0, 0);
            check_temp_hum();
            read_ph();
            sprintf(buffer, "Water level: %d", currentWaterLevel);
            lcd_moveto(2, 0);
            lcd_stringout(buffer);
            break;
        case MENU_VIEW:
        		sci_out(0xfe);              // Clear the screen
      			sci_out(0x51);
            lcd_moveto(0, 0);
            lcd_stringout("1: pH Level         ");
            lcd_moveto(1, 0);
            lcd_stringout("2: Nutrient Level   ");
            lcd_moveto(2, 0);
            lcd_stringout("3: Water Level      ");
            lcd_moveto(3, 0);
            lcd_stringout("4: Go Back          ");
            // Indicate selected item
            //lcd_moveto(menuIndex, 19);
            //lcd_stringout("<");
            break;
        case EDIT_PH_LEVEL:
        		sci_out(0xfe);              // Clear the screen
      			sci_out(0x51);
      			int pHLevelInt = (int)(pHLevel * 10); // Convert pH level to an integer after multiplying by 10
						sprintf(buffer, "Set pH: %d.%d", pHLevelInt / 10, pHLevelInt % 10); // Print the integer part and the remainder as the fractional part
            lcd_moveto(0, 0);
            lcd_stringout(buffer);
            break;
        case EDIT_NUTRIENT_LEVEL:
        		sci_out(0xfe);              // Clear the screen
      			sci_out(0x51);
            sprintf(buffer, "Set Nutrients: %d", nutrientLevel);
            lcd_moveto(0, 0);
            lcd_stringout(buffer);
            break;
        case EDIT_WATER_LEVEL:
        		sci_out(0xfe);              // Clear the screen
      			sci_out(0x51);
            sprintf(buffer, "Set Water Level: %d", desiredWaterLevel);
            lcd_moveto(0, 0);
            lcd_stringout(buffer);
            break;
    }
}

void read_water_levels() {
    // Simulated water level reading; replace with actual ADC code
    currentWaterLevel = 0;
    if (adc_read_channel(0) > 100) currentWaterLevel++;
    if (adc_read_channel(1) > 100) currentWaterLevel++;
    if (adc_read_channel(2) > 100) currentWaterLevel++;
    updateDisplay();
    control_water_pump();
}

void control_water_pump() {
    if (currentWaterLevel < desiredWaterLevel) {
        PORTB |= (1 << PB7); // Turn on the water pump
    } else {
        PORTB &= ~(1 << PB7); // Turn off the water pump
    }
}

void rotary_init(void) {
    // Set PD2 (INT0) and PD4 as input
    DDRD &= ~((1 << PD2) | (1 << PD4));
    // Enable pull-up resistors on PD2 and PD4
    PORTD |= (1 << PD2) | (1 << PD4);
    
    // Set PD3 as input for the pushbutton
    DDRD &= ~(1 << PD3);
    PORTD |= (1 << PD3); // Enable pull-up
    
    // External Interrupt Request 0 Enable (INT0)
    EIMSK |= (1 << INT0);
    // Set INT0 to trigger on any logical change
    EICRA |= (1 << ISC00);
    
    // Enable INT1 for pushbutton (PD3)
    EIMSK |= (1 << INT1);
    // Trigger on falling edge (pushbutton press)
    EICRA |= (1 << ISC11);
}