/*
 * uart.c
 *
 * Created: 2/24/2025 12:32:39
 *  Author: Student
 */ 
#include <stdio.h>
#include <avr/io.h>

void UART_init(uint16_t Baudrate){
		uint16_t ubrr;
		ubrr = F_CPU / 16 / Baudrate-1;
		// Setting Baud rate
		UBRR1H = (uint8_t)(ubrr>>8);
		UBRR1L = (uint8_t)ubrr;
		// Enable Tx and RX
		UCSR1B = (1<<RXEN1)|(1<<TXEN1);		// Setting frame format (Async, Parity-Disabled, 1 Stop bit, 8 data bits)
}

void UART_SendChar(uint8_t data){
	// Wait for empty transmit buffer
	while ( !( UCSR1A & (1<<UDRE1)) );

	UDR1 = data;
}

void UART_SendString(char *text){
	// Wait for empty transmit buffer
	while ( !( UCSR1A & (1<<UDRE1)) );
	
	while (*text != 0x00){
		UART_SendChar(*text);
		text++;
	}
}

uint8_t UART_GetChar(void){
	// Wait for received char
	while ( ! (UCSR1A & ( 1 << RXC1) ) );
	
	return UDR1;
}


int printCHAR(char character ,FILE *stream){
while ((UCSR1A & (1 << UDRE1)) == 0){};
UDR1 = character;
return 0 ;
}
