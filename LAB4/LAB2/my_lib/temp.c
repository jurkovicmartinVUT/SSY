
/*
 * temp.c
 *
 * Created: 3/10/2025 12:34:42
 *  Author: Student
 */ 
#include <stdio.h>
#include <avr/io.h>
#include "temp.h"
#include "i2c.h"
#include "uart.h"

void at30_init(void){
	i2cInit();
}
uint8_t at30_setPrecision(uint8_t prec){
	uint_fast16_t config_register = 0;
	// config_register |= (uint16_t) (prec << R0);
	config_register |= (uint16_t) (prec << 5);
	i2cStart();
	i2cWrite(TempSensorAddrW);
	if (i2cGetStatus() != 0x18){
		UART_SendString("Error 18\n\r");
		return 0;
	};
	i2cWrite(Temp_configRegister);
	if (i2cGetStatus() != 0x28){
		UART_SendString("Error 28\n\r");
		return 0;
	};
	i2cWrite((uint8_t) (config_register >> 8));
	if (i2cGetStatus() != 0x28){
		UART_SendString("Error 28\n\r");
		return 0;
	};
	i2cWrite((uint8_t) (config_register));
	if (i2cGetStatus() != 0x28){
		UART_SendString("Error 28\n\r");
		return 0;
	};
	i2cStop();
	return 1;
}


float at30_readTemp(void){	volatile uint8_t buffer[2];	// float teplota	volatile int16_t teplotaTMP;		// Do pointer registru nastavime co chceme cist - Temp registr	i2cStart();	i2cWrite(TempSensorAddrW);	if (i2cGetStatus() != 0x18){		UART_SendString("Error 18\n\r");	};	i2cWrite(Temp_tempRegister);	if (i2cGetStatus() != 0x28){		UART_SendString("Error 28\n\r");	};	i2cStop();		// Cteme dany temp registr (start/adresa/cteme 2 byty)	i2cStart();	if (i2cGetStatus() != 0x08){		UART_SendString("Error 18\n\r");	};	i2cWrite(TempSensorAddrR);	if (i2cGetStatus() != 0x40){		UART_SendString("Error 40\n\r");	};	// 8b	buffer[0] = i2cReadACK();	if (i2cGetStatus() != 0x50){		UART_SendString("Error 50\n\r");	};	// 8b	buffer[1] = i2cReadNACK();	if (i2cGetStatus() != 0x58){		UART_SendString("Error 58\n\r");	};		i2cStop();	// 2 byty ulozime ve spravnem poradi do integeru	teplotaTMP = (buffer[0] << 8) | buffer[1];	// vydelime 256 (odstraneni sponich 8 bitu- je desetinne mosti - viz datasheet)	return (float) teplotaTMP / 256;}