
/*
 * button.c
 *
 * Created: 3/10/2025 12:28:57
 *  Author: Student
 */ 
#include <stdio.h>
#include <avr/io.h>
#include "avr/interrupt.h"
#include "makra.h"

void init_button1(void) {
	// POLLING
	
	// Set PE5 as input
	cbi(DDRE, 5);
	// Enable pull-up
	sbi(PORTE, 5);
}


uint8_t is_button1_pressed(void) {
	// POLLING
	
	return !(PINE & (1 << PINE5));
}


void init_button1_interrupt(void) {
	// INTERRUPT
	
	// Set PE5 as input
	cbi(DDRE, 5);
	// Enable pull-up
	sbi(PORTE, 5);

	// Configure External Interrupt INT5
	// Falling edge detection
	EICRB |= (1 << ISC51);
	EICRB &= ~(1 << ISC50);
	// Enable INT5 interrupt
	EIMSK |= (1 << INT5);

	// Enable global interrupts
	sei();
}