#include <avr/io.h>
#include "adcph.h"

void adc_init_ph(void) {
    // AVCC with external capacitor at AREF pin, left adjust result
    ADMUX |= (1<<REFS0);
    ADMUX &= ~(1<<ADLAR);
    // Select ADC1 (pin 24)
    ADMUX |= (1<<MUX0);
    
    // Enable ADC, set prescaler to 128
    ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
    ADCSRA |= (1<<ADEN);
}

// Read the ADC value
uint16_t adc_read_ph(void) {
    ADCSRA |= (1<<ADSC); // Start conversion
    while ((ADCSRA & (1<<ADSC)) != 0); // Wait for conversion to complete
    return (ADCL | (ADCH << 8)); // Return the 10-bit result
}