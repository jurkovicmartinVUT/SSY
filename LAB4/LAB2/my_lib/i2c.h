
/*
 * i2c.h
 *
 * Created: 3/10/2025 12:18:36
 *  Author: Student
 */ 
#include <stdio.h>

#ifndef I2C_H_
#define I2C_H_

void i2cInit(void);
void i2cStart(void);
void i2cStop(void);
void i2cWrite(uint8_t byte);
uint8_t i2cReadACK(void);
uint8_t i2cReadNACK(void);
uint8_t i2cGetStatus(void);

#endif