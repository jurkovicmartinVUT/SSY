
/*
 * timer.c
 *
 * Created: 3/3/2025 12:24:00
 *  Author: Student
 */

#include <stdio.h>
#include <avr/io.h>
#include "timer.h"
#include "avr/interrupt.h"

void Timer1_cmp_start(uint16_t porovnani) {
	cli(); // zakazat preruseni
	TCCR1A = 0; // vycistit kontrolni registry
	TCCR1B = 0; //
	TIMSK1 = 0; //
	// nastavit hodnotu pro porovnani
	OCR1A = porovnani;
	// CTC mod :
	TCCR1B |= ( 1 << WGM12);
	// 1024 preddelicka :
	// TCCR1B |= ( 1 << CS10 ) ;
	// TCCR1B |= ( 1 << CS12 ) ;
	TCCR1B |= PRESCALE_mask;
	// povolit preruseni, pokud budete POTREBOVAT:
	TIMSK1 |= ( 1 << OCIE1A);
	// vystup na pin OC1A, toggle
	TCCR1A |= ( 1 << COM1A0);
	sei(); // povolit globalni preruseni
}

void Timer0_ovf_start() {
	cli ( ) ; // z a k a z a t p r e r u s e n i
	TCCR0A = 0 ; // v y c i s t i t k o n t r o l n i r e g i s t r y
	TCCR0B = 0 ;
	TIMSK0 = 0 ; //
	//Nastav_timer ( mil s ) ;
	// n a s t a v i t p o c a t e c ni hodnotu
	//TCNT0=255?Nastav_timer ( mil s ) ;
	// 1024 p r e d d eli c k a :
	TCCR0B |= PRESCALE_mask;
	// vystup na pin OC0A, t o g g l e
	TCCR0A |= ( 1 << COM0A0) ;
	// p o v o l i t p r e r u s e n i :
	TIMSK0 |= ( 1 << TOIE0 ) ;
	sei( ) ; // p o v o l i t g l o b a l n i p r e r u s e n i
}

void Timer0Stop() {
	TCCR0B=0;
}


void Timer2_fastpwm_start ( uint8_t strida ) {
	cli() ; // z a k a z a t p r e r u s e n i
	TCCR2A = 0 ; // v y c i s t i t k o n t r o l n i r e g i s t r y
	TCCR2B = 0 ;
	TIMSK2 = 0 ; //
	// n a s t a v i t hodnotu pro PWM
	OCR2A = (255 * strida) / 100;
	// fastpwm mod:
	TCCR2A |= ( 1 << WGM21) ;
	TCCR2A |= ( 1 << WGM20) ;
	// 1024 p r e d d eli c k a :
	TCCR2B |= PRESCALE_mask ;
	// p o v o l i t p r e r u s e ni , pokud bude pot ? eba . . . :
	// TIMSK2 |= ( 1 << TOIE2 ) ;
	// vystup na pin OC2A
	TCCR2A |= ( 1 << COM2A1) ;
	sei() ; // p o v o l i t g l o b a l n i p r e r u s e n i
}