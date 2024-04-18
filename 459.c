#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

#include "lcd459.h"
#include "i2c.h"
#include "adc459.h"
#include "temphum.h"


#define FOSC 7372800
#define BDIV (FOSC / 100000 - 16) / 2 + 1

typedef enum {
    DEFAULT_VIEW,
    MENU_VIEW,
    EDIT_PH_LEVEL,
    EDIT_NUTRIENT_LEVEL,
    EDIT_WATER_LEVEL
} SystemState;

volatile SystemState state = DEFAULT_VIEW;
volatile int menuIndex = 0;
volatile float pHLevel = 6.5;
volatile int nutrientLevel = 2;
volatile int desiredWaterLevel = 3;
volatile int currentWaterLevel = 0;
volatile unsigned long lastInteraction;
volatile int MENU_ITEMS = 3;

void sci_init(void);
void lcd_init(void);
void rotary_init(void);
void adc_init(void);
void check_temp_hum(void);
void updateDisplay(void);
void read_water_levels(void);
void control_water_pump(void);

uint8_t i2c_io(uint8_t, uint8_t *, uint16_t, uint8_t *, uint16_t);
void i2c_init(uint8_t);

ISR(INT0_vect) {
    static uint8_t lastA = 0;
    static uint8_t lastB = 0;
    uint8_t currentA = (PIND & (1 << PD2)) != 0;
    uint8_t currentB = (PIND & (1 << PD4)) != 0;  // Assuming B is on PD4

    if (currentA != lastA) {  // Only act on changes of A
        if (currentA == currentB) {
            // Counter-clockwise rotation
            if (state == MENU_VIEW) {
                menuIndex--;
                if (menuIndex < 0) menuIndex = MENU_ITEMS - 1;  // Wrap around
            } else if (state == EDIT_PH_LEVEL) {
                pHLevel -= 0.1;
                if (pHLevel < 3.0) pHLevel = 3.0;  // Clamp the pH level
            } else if (state == EDIT_NUTRIENT_LEVEL) {
                nutrientLevel--;
                if (nutrientLevel < 0) nutrientLevel = 0;  // Clamp the nutrient level
            } else if (state == EDIT_WATER_LEVEL) {
                desiredWaterLevel--;
                if (desiredWaterLevel < 1) desiredWaterLevel = 1;  // Clamp the water level setting
            }
            updateDisplay();  // Refresh the LCD based on the current state
        } else {
            // Clockwise rotation
            if (state == MENU_VIEW) {
                menuIndex++;
                if (menuIndex >= MENU_ITEMS) menuIndex = 0;  // Wrap around
            } else if (state == EDIT_PH_LEVEL) {
                pHLevel += 0.1;
                if (pHLevel > 10.0) pHLevel = 10.0;  // Clamp the pH level
            } else if (state == EDIT_NUTRIENT_LEVEL) {
                nutrientLevel++;
                if (nutrientLevel > 20) nutrientLevel = 20;  // Clamp the nutrient level
            } else if (state == EDIT_WATER_LEVEL) {
                desiredWaterLevel++;
                if (desiredWaterLevel > 3) desiredWaterLevel = 3;  // Clamp the water level setting
            }
        updateDisplay();  // Refresh the LCD based on the current state
      		}
    }
    else if (currentB != lastB) {  // Only act on changes of A
        if (currentA == currentB) {
            // Clockwise rotation
            if (state == MENU_VIEW) {
                menuIndex++;
                if (menuIndex >= MENU_ITEMS) menuIndex = 0;  // Wrap around
            } else if (state == EDIT_PH_LEVEL) {
                pHLevel += 0.1;
                if (pHLevel > 10.0) pHLevel = 10.0;  // Clamp the pH level
            } else if (state == EDIT_NUTRIENT_LEVEL) {
                nutrientLevel++;
                if (nutrientLevel > 20) nutrientLevel = 20;  // Clamp the nutrient level
            } else if (state == EDIT_WATER_LEVEL) {
                desiredWaterLevel++;
                if (desiredWaterLevel > 3) desiredWaterLevel = 3;  // Clamp the water level setting
            }
            updateDisplay();  // Refresh the LCD based on the current state
        } else {
            // Counter-clockwise rotation
            if (state == MENU_VIEW) {
                menuIndex--;
                if (menuIndex < 0) menuIndex = MENU_ITEMS - 1;  // Wrap around
            } else if (state == EDIT_PH_LEVEL) {
                pHLevel -= 0.1;
                if (pHLevel < 3.0) pHLevel = 3.0;  // Clamp the pH level
            } else if (state == EDIT_NUTRIENT_LEVEL) {
                nutrientLevel--;
                if (nutrientLevel < 0) nutrientLevel = 0;  // Clamp the nutrient level
            } else if (state == EDIT_WATER_LEVEL) {
                desiredWaterLevel--;
                if (desiredWaterLevel < 1) desiredWaterLevel = 1;  // Clamp the water level setting
            }
        }
      
        updateDisplay();  // Refresh the LCD based on the current state
    }
    lastA = currentA;
    lastB = currentB;
}

ISR(INT1_vect) {
    // ISR for pushbutton to select menu items or confirm settings
    //lastInteraction = millis();
    switch (state) {
        case DEFAULT_VIEW:
            state = MENU_VIEW;
            break;
        case MENU_VIEW:
            if (menuIndex == 0) {
                state = EDIT_PH_LEVEL;
            } else if (menuIndex == 1) {
                state = EDIT_NUTRIENT_LEVEL;
            } else {
                state = EDIT_WATER_LEVEL;
            }
            break;
        case EDIT_PH_LEVEL:
        case EDIT_NUTRIENT_LEVEL:
        case EDIT_WATER_LEVEL:
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
    PORTC |= (1<<PC5) | (1<<PC4); //initialize for i2c

    //DDRB |= (1 << PB7); // Configure PB7 as output for relay
    sei();  // Enable global interrupts
    //lastInteraction = millis();
}

void loop() {
    if (state == DEFAULT_VIEW) {
        check_temp_hum();
        read_water_levels();
        /*if (millis() - lastInteraction > 20000) {  // No interaction for 20s
            state = DEFAULT_VIEW;
            updateDisplay();
        }*/
    }
    _delay_ms(100);  // Main loop delay
}

int main(void) {
    setup();
    while (1) {
        loop();
    }
    return 0;
}

void updateDisplay() {
    char buffer[20];
    switch (state) {
        case DEFAULT_VIEW:
            lcd_moveto(0, 0);
            check_temp_hum();
            sprintf(buffer, "Water level: %d", currentWaterLevel);
            lcd_moveto(2, 0);
            lcd_stringout(buffer);
            break;
        case MENU_VIEW:
            lcd_moveto(0, 0);
            lcd_stringout("1: pH Level         ");
            lcd_moveto(1, 0);
            lcd_stringout("2: Nutrient Level   ");
            lcd_moveto(2, 0);
            lcd_stringout("3: Water Level      ");
            // Indicate selected item
            lcd_moveto(menuIndex, 19);
            lcd_stringout("<");
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
