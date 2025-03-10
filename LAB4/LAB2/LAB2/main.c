/*
 * LAB2.c
 *
 * Created: 2/24/2025 12:03:25
 * Author : Student
 */ 

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "my_lib/uart.h"
#include "my_lib/makra.h"

void board_init(){
	UART_init(38400);
}


void print_menu(){
	UART_SendString("\n\r\n\rMENU:\n\r");
	UART_SendString("1 = Print lower alphabet\n\r");
	UART_SendString("2 = Print upper alphabet\n\r");
	UART_SendString("3 = Blink 3x with LED 2\n\r");
	UART_SendString("4 = Clear terminal\n\r");
	UART_SendString("0 = Exit\n\r\n\r");
}


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


ISR(INT5_vect) {
	// INTERRUPT handling
    if (!(PINE & (1 << PINE5))) {
        UART_SendChar('1');
    }
}


int main(void)
{
    board_init();
	init_button1_interrupt();
	
	int run = 1;
	char choice;
	while(run){
		print_menu();
		choice = UART_GetChar();
		switch (choice){
			case '0':
				run = 0;
				UART_SendString("Quiting");
				break;
			case '1':
				for (char c = 'a'; c <= 'z'; c++){
					UART_SendChar(c);
					UART_SendChar(' ');
				}
				break;
			case '2':
				for (char c = 'A'; c <= 'Z'; c++){
					UART_SendChar(c);
					UART_SendChar(' ');
				}
				break;
			case '3':
				UART_SendString("Blinking with LED 2");
				sbi(DDRB, DDB6);
				for (int i =0; i<3; i++){
					sbi(PORTB, PB6); 		    
					_delay_ms(500);  		    
					cbi(PORTB, PB6);
					_delay_ms(500);
				}
				cbi(DDRB, DDB6);
				break;
			case '4':
				// Escape = \1xb
				UART_SendString("\x1b[2J");
				break;
			default:
				UART_SendString("Invalid option");
				break;
		}
		
	}
	
	/*
	// POLLING
	init_button1();
	while(1){
		if (is_button1_pressed()) {
            UART_SendChar('x');
        }
	}
	*/
	
	return 0;
}

