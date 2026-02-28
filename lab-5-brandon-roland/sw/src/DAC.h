// File **********DAC.h***********
// Lab 5
// Programs to interface with Switch buttons   
// Spring 2025

#ifndef DAC_H
#define DAC_H

#include <stdint.h>

void dac_init(void);

int dac_output(uint16_t data);

#endif
