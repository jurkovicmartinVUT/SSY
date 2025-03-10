/*
LAB 3
 */ 

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "my_lib/uart.h"
#include "my_lib/makra.h"
#include "my_lib/timer.h"
#include "my_lib/button.h"
#include "my_lib/temp.h"
#include "my_lib/adc.h"

// Choice 5 output frequency
#define FREQUENCY 2


FILE uart_str = FDEV_SETUP_STREAM( printCHAR , NULL, _FDEV_SETUP_RW);

void board_init(){
	UART_init(38400);
	// Working printf
	stdout = &uart_str;
}

ISR (INT5_vect) {
	// Button
	// INTERRUPT handling
    if (!(PINE & (1 << PINE5))) {
        UART_SendChar('1');
    }
}

ISR (TIMER1_COMPA_vect){
}

void print_menu(){
	UART_SendString("\n\r\n\rMENU:\n\r");
	UART_SendString("1 = Print lower alphabet\n\r");
	UART_SendString("2 = Print upper alphabet\n\r");
	UART_SendString("3 = Blink 3x with LED 2\n\r");
	UART_SendString("4 = Clear terminal\n\r");
	UART_SendString("5 = Generate 2 Hz\n\r");
	UART_SendString("+ = Increase brightness of LED0\n\r");
	UART_SendString("- = Decrease brightness of LED0\n\r");
	UART_SendString("t = Read temperature from I2C sensor\n\r");
	UART_SendString("l = Get light intensity from photodetector\n\r");
	UART_SendString("0 = Exit\n\r\n\r");
}


int main(void)
{
    board_init();
	init_button1_interrupt();
	
	int run = 1;
	char choice;
	
	// 0 - 100 control brightness of LED0
	uint8_t duty = 50;
	sbi(DDRB, PB4);
	Timer2_fastpwm_start(duty);
	
	// For formatted strings
	char buffer[30];
	
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
			case '5':
				sbi(DDRB, PB5);
				UART_SendString("Generating 2 Hz");
				// Timer register comparator
				uint16_t compare = (F_CPU / (2 * PRESCALE_value * FREQUENCY)) - 1;
				Timer1_cmp_start(compare);
				break;
			case '+':
				if (duty > 0){
					duty = duty - 10;
				}
				Timer2_fastpwm_start(duty);
				UART_SendString("Brightness of LED 0 increased");
				break;
			case '-':
				if (duty < 100){
					duty = duty + 10;
				} 
				Timer2_fastpwm_start(duty);
				UART_SendString("Brightness of LED 0 decreased");
				break;
			case 't':
					at30_init();
					// 12 bit resolution
					at30_setPrecision(0b11);
					float temp = at30_readTemp();
					
					// Send the temperature
					sprintf(buffer, "Temperature is: %.2f\n\r", temp);
					UART_SendString(buffer);
				break;
			case 'l':
				// 64 = preddelicka, 1 = ref U (~5V) (3 for weak signal (~1,1V)
				ADC_Init(64 ,1);
				// Bigger intensity = smaller A/D value
				// Channel 3 corresponds to EXT3 pins
				uint16_t converted_intensity = ADC_get(3);
				ADC_stop();
				
				// Send the intensity
				sprintf(buffer, "Intensity is: %2d\n\r", converted_intensity);
				UART_SendString(buffer);
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

