// File **********DAC.c***********
// Lab 5
// Programs to interface with Switch buttons   
// Spring 2025

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "./DAC.h"

void dac_init() {
    /**
     * Unified_Port_Init in Lab5.c calls Port_D_Init, which initializes the Port
     * D GPIOs for the appropriate alternate functionality (SSI).
     *
     * According to Table 15-1. SSI Signals in the datasheet, this corresponds
     * to SSI1. The corresponding Valvanoware register defines are at L302 and
     * L2670 in inc/tm4c123gh6pm.h. Use this in combination with the datasheet
     * or any existing code to write your driver! An example of how to
     * initialize SSI is found in L741 in inc/ST7735.c.
     */
    SYSCTL_RCGCSSI_R |= 0x02;       // Activate SSI1
    while((SYSCTL_PRSSI_R & 0x02) == 0){};
    SYSCTL_RCGCGPIO_R |= 0x08;      // Activate Port D
    while((SYSCTL_PRGPIO_R & 0x08) == 0){}; // Wait for Port D

    // 2. Configure Port D Pins (PD0: SCLK, PD1: CS, PD3: MOSI)
    GPIO_PORTD_AFSEL_R |= 0x0B;     // Enable alt funct on PD0,1,3
    GPIO_PORTD_DEN_R |= 0x0B;       // Enable digital I/O on PD0,1,3
    GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0xFFFF0F00) + 0x00002022; // SSI1
    GPIO_PORTD_AMSEL_R = 0;         // Disable analog functionality

    // 3. Configure SSI1
    SSI1_CR1_R &= ~SSI_CR1_SSE;     // Disable SSI to configure
    SSI1_CR1_R &= ~SSI_CR1_MS;      // Set as Master
  
    // Clock settings for 80MHz SysClk to get 8MHz SSI Clock
    // SysClk / (CPSDVSR * (1 + SCR)) -> 80 / (10 * (1 + 0)) = 8MHz
    SSI1_CPSR_R = 10;               // Must be even
  
    // SCR=1, SPH=0, SPO=0, Freescale Format, 16-bit Data
    // 0x0F = 1111 (16-bit data size)
    SSI1_CR0_R = 0x004F;

    SSI1_CR1_R |= SSI_CR1_SSE;      // Enable SSI1
}

int dac_output(uint16_t data) {
    // An example of how to send data via SSI is found in L534 of inc/ST7735.c.
    // Remember that 4 out of the 16 bits is for DAC operation. The last 12 bits
    // are for data. Read the datasheet! 
    // 1. Wait until transmit FIFO is not full
    while((SSI1_SR_R & SSI_SR_TNF) == 0){}; 

    // 2. Format: [4 bits Control][12 bits Data]
    // 0x4000 sets bit 14 (SPD=1 for fast mode) and bit 12 (write to DAC A)
    // Mask data to 12 bits just in case (0x0FFF)
    SSI1_DR_R = 0x4000 | (data & 0x0FFF);

    return 1;
}
