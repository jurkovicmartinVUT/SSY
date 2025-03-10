
/*
 * adc.h
 *
 * Created: 3/10/2025 13:35:32
 *  Author: Student
 */
#include <stdio.h>

#ifndef ADC_H_
#define ADC_H_

void ADC_Init(uint8_t prescale, uint8_t uref);
uint16_t ADC_get(uint8_t chan);
void ADC_stop(void);

#endif

