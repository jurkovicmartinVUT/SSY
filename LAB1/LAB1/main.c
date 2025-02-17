/*
 * LAB1.c
 *
 * Created: 02.02.2020 9:01:38
 * Author : Ondra
 */ 

/************************************************************************/
/* INCLUDE                                                              */
/************************************************************************/
#include <avr/io.h>
#include <util/delay.h>
#include "libs/libprintfuart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libs/alphabet.h"
/************************************************************************/
/* DEFINES                                                              */
/************************************************************************/
#define CONST 2
// #define ODECET
#define sbi(var, mask)  ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)  ((var) &= (uint8_t)~(1 << mask))
#define tbi(var, mask)	(var & (1 << mask) )
#define xbi(var, mask)	((var)^=(uint8_t)(1 << mask))

// F_CPU definovano primo v projektu!!! Debug->Properties->Toolchain->Symbols

/************************************************************************/
/* VARIABLES                                                            */
/************************************************************************/
char field[30];

//musime vytvorit soubor pro STDOUT
FILE uart_str = FDEV_SETUP_STREAM(printCHAR, NULL, _FDEV_SETUP_RW);

/************************************************************************/
/* PROTOTYPES                                                           */
/************************************************************************/

void board_init();

/************************************************************************/
/* FUNCTIONS                                                            */
/************************************************************************/
void board_init(){
	UART_init(38400); //nastaveni rychlosti UARTu, 38400b/s
	stdout = &uart_str; //presmerovani STDOUT
}


int main(void)
{ 	
	board_init();
	_delay_ms(1000);
	printf("\n\rHello word\n\r");
    _delay_ms(1000);
	
	// UKOL 4
	int a = 10;
	#ifdef ODECET
		a = a - CONST;
	#endif
	printf("Value of a: %d \n\r", a);
	
	// UKOL 5
	unsigned char b = 255;
	unsigned char c = 255;
	short d;
	
	d = (short) b + (short) c;
	printf("Value of d: %i \n\r", d);
	
	// UKOL 6
	int e = 24;
	e = e >> 3;
	e = e - 1;
	e = e & 0x2;
	printf("Value of e: %d \n\r", e);
	
	// UKOL 7
	int f = 200;
	char str[] = "Hodnota=";

	char f_str[4];
	itoa(f, f_str, 10); 
	char text_string[20];
	strcpy(text_string, str);
	strcat(text_string, f_str);
	printf("String output: %s \n\r", text_string);
	
	char text_sprintf[20];
	sprintf(text_sprintf, "Hodnota=%d", f);
	printf("Sprintf output: %s \n\r", text_sprintf);
	
	// UKOL 8
	generateField(NORMAL_CASE);
	printField();
	capsLetters();
	printField();
	
	// UKOL 9
	int g = 12;
	int *pointer;
	pointer = &g;
	printf("Value of g: %d \n\r", *pointer);
	printf("Address of g: %d \n\r", pointer);
	
	// UKOL 11	/*
	DDRB |= (1 << DDB6);
    while (1)
    {		   
	    PORTB &= ~(1 << PORTB6);  		    
	    _delay_ms(500);  		    
	    PORTB |= (1 << PORTB6);   
	    _delay_ms(500);
    }	*/
	//UKOL 12
	DDRB |= (1 << DDB5) | (1 << DDB6);
	DDRE |= (1 << DDB3);
	while(1){
		PORTB = 0b01100000;
		PORTE = 0b00001000;
		_delay_ms(500);
		PORTB = 0b00000000;
		PORTE = 0b00000000;
		_delay_ms(500);

		PORTB = 96;
		PORTE = 8;
		_delay_ms(500);
		PORTB = 0;
		PORTE = 0;
		_delay_ms(500);

		PORTB = 0x60;
		PORTE = 0x08;
		_delay_ms(500);
		PORTB = 0x00;
		PORTE = 0x00;
		_delay_ms(500);

		sbi(PORTB, PB5);
		sbi(PORTB, PB6);
		sbi(PORTE, PE3);
		_delay_ms(500);
		cbi(PORTB, PB5);
		cbi(PORTB, PB6);
		cbi(PORTE, PE3);
		_delay_ms(500);

		xbi(PORTB, PB5);
		xbi(PORTB, PB6);
		xbi(PORTE, PE3);
		_delay_ms(500);
	}
	
	/*
	int i=0;
    while (1) 
    {
	_delay_ms(10000);
	i++;
	printf("Test x = %d \n\r", i);
    }
	*/
	return 0;
}

