
/*
 * temp.h
 *
 * Created: 3/10/2025 12:34:56
 *  Author: Student
 */
#include <stdio.h>

#ifndef TEMP_H_
#define TEMP_H_

#define TempSensorAddrR 0b10010111
#define SerialEEPROMAddrR 0b10100111
#define TempSensorAddrW 0b10010110
#define SerialEEPROMAddrW 0b10100110#define Temp_configRegister 0x01#define Temp_tempRegister 0x00
void at30_init(void);
uint8_t at30_setPrecision(uint8_t prec);
float at30_readTemp(void);

#endif
