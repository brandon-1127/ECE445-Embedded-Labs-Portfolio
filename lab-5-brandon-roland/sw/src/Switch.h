// File: Switch.h
// Description: Header file for external button interface on Port C
// Buttons: PC4, PC5 (Positive logic with external pull-down resistors)

#ifndef __SWITCH_H__
#define __SWITCH_H__

#include <stdint.h>

/**
 * @details Initialize Port C pins PC4 and PC5 for input with interrupts.
 * Configured for positive logic (rising edge trigger).
 * PC4: Used for clock/alarm control logic.
 * PC5: Used for clock/alarm control logic.
 * @note    Requires external pull-down resistors.
 */
void Switch_Init(void);

/**
 * @details Interrupt Service Routine for Port C.
 * Handles debouncing and state transitions for Clock, Setting, 
 * and Alarm modes.
 */
void GPIOPortC_Handler(void);

#endif // __SWITCH_H__