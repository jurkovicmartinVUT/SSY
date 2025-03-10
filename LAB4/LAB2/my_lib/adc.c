
/*
 * adc.c
 *
 * Created: 3/10/2025 13:35:18
 *  Author: Student
 */
#include <stdio.h>
#include <avr/io.h>
#include "makra.h"

void ADC_Init(uint8_t prescale, uint8_t uref){
	ADMUX = 0;
	ADCSRA = 0;
	ADCSRA |= (prescale << ADPS0);
	ADMUX |= (uref << REFS0);
	// Kontrola jestli jsou napeti OK
	sbi(ADCSRA, ADEN);
	while(!(ADCSRB & 0x80));
	while(!(ADCSRB & 0x20));
}


uint16_t ADC_get(uint8_t chan){
	uint16_t tmp = 0;
	// smazat MUX
	ADMUX &= ~(31 << MUX0);
	ADCSRB &= ~(1 << MUX5);
	
	// nastavit spravny kanal
	ADMUX |= (chan << MUX0);
	
	// spusti konverzi
	ADCSRA |= (1 << ADSC);
	// pockat nez skonci
	while ((tbi(ADCSRA, ADSC))){}
	tmp = ADC;
	
	// vynulovat priznak konverze
	ADCSRA |= (1 << ADIF);
	
	return tmp;
}


void ADC_stop(void){
	cbi(ADCSRA, ADEN);
}
