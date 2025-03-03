
/*
 * timer.h
 *
 * Created: 3/3/2025 12:24:09
 *  Author: Student
 */ 
#include <stdio.h>

#ifndef TIMER_H_
#define TIMER_H_
#define PRESCALE_mask 5
#define PRESCALE_value 1024

void Timer1_cmp_start(uint16_t porovnani);
void Timer0_ovf_start();
void Timer0Stop();
void Timer2_fastpwm_start (uint8_t strida);

#endif