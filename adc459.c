#include <avr/io.h>
#include "adc459.h"

void adc_init(void) {
    // AVCC with external capacitor at AREF pin, left adjust result
    ADMUX |= (1<<REFS0);
    ADMUX |= (1<<ADLAR);
    
    // Enable ADC, set prescaler to 128
    ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
    ADCSRA |= (1<<ADEN);
}


unsigned char adc_read_channel(uint8_t channel) {
    // Clear the previously selected channel
    ADMUX &= 0xF0; 
    // Set the desired channel
    ADMUX |= (channel & 0x0F); 
    ADCSRA |= (1 << ADSC); // Start conversion
    while (ADCSRA & (1 << ADSC)); // Wait for conversion to complete
    return ADCH; // Return the 8-bit result
}