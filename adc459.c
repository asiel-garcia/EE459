#include <avr/io.h>
#include "adc459.h"

void adc_init(void) {
    // AVCC with external capacitor at AREF pin, left adjust result
    ADMUX |= (1<<REFS0);
    ADMUX |= (1<<ADLAR);
    // Select ADC3 (pin 26)
    ADMUX |= (1<<MUX1) | (1<<MUX0);
    
    // Enable ADC, set prescaler to 128
    ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
    ADCSRA |= (1<<ADEN);
}

// Read the ADC value
unsigned char adc_read(void) {
    ADCSRA |= (1<<ADSC); // Start conversion
    while ((ADCSRA & (1<<ADSC)) != 0); // Wait for conversion to complete
    return ADCH; // Return the 8-bit result
}
