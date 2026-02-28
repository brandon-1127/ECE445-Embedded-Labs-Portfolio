#ifndef __LAB3_H__
#define __LAB3_H__

#include <stdint.h>

void Clocking(void);

void Draw_Clock(uint32_t hours, uint32_t minutes, uint32_t seconds, 
                uint32_t genmode, uint32_t settingmode, uint32_t index_min_hour);

void Display_Digits(uint32_t digits);

#endif
