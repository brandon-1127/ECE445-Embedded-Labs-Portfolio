#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

void (*PeriodicTask0)(void); 

void Timer0A_Init(void(*task)(void), uint32_t period, uint32_t priority);

void Timer0A_Handler();

void Timer0A_Stop();

void Timer1A_Init(uint32_t period, uint32_t priority);

void Timer1A_Handler();

#endif
