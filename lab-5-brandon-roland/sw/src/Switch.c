// File **********Switch.c***********
// Spring 2026

// define your hardware interface

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "Switch.h"
#include "music.h"

// Debounce timing: 5ms debounce delay
#define DEBOUNCE_TIME 400000  // 5ms at 80MHz clock

static uint32_t firstPress = 0; // 0 means never started, 1 means started at least once

// Debounce helper function - simple busy-wait delay
static void Debounce_Delay(void) {
  volatile uint32_t delay = DEBOUNCE_TIME;
  while(delay--) {}
}

// Debounce helper - verify button state is stable
// Returns 1 if button is still pressed, 0 if it bounced
static uint32_t Debounce_VerifyPortC(uint32_t pins) {
  Debounce_Delay();
  return (GPIO_PORTC_DATA_R & pins);  // Returns non-zero (True) if still pressed
}

void Switch_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x04;     // Activate Port C
  while((SYSCTL_PRGPIO_R & 0x04) != 0x04){};

  // Port C Setup (External Buttons PC4-5)
  GPIO_PORTC_DIR_R &= ~0x30;     //input
  GPIO_PORTC_AFSEL_R &= ~0x30;   //disable other functions
  GPIO_PORTC_PUR_R &= ~0x30;     //disable pull-ups for PC4-5
  GPIO_PORTC_DEN_R |= 0x30;      //enable digital
  
  // Port C Interrupt Configuration
  GPIO_PORTC_IS_R &= ~0x30;      // Edge-sensitive
  GPIO_PORTC_IBE_R &= ~0x30;     // Not both edges
  GPIO_PORTC_IEV_R |= 0x30;     // Rising edge (pressed)
  GPIO_PORTC_ICR_R = 0x30;       // Clear flags
  GPIO_PORTC_IM_R |= 0x30;       // Arm interrupts
  NVIC_PRI0_R = (NVIC_PRI0_R & 0xFF00FFFF) | 0x00A00000; // Priority 5
  NVIC_EN0_R = 0x00000004;  // Enable interrupt 2 in NVIC (Port C)    
}

void GPIOPortC_Handler(void){
  uint32_t interrupted = GPIO_PORTC_RIS_R; // read hardware flags
  GPIO_PORTC_ICR_R = 0x30;      // acknowledge PC4, PC5

  // Only process if the debounce period has passed
    if(interrupted & 0x10){     // PC4 (Play/Pause/Start)
      if(firstPress == 0){
          Music_Start(CurrentPlaylistIndex); // Initial start
          firstPress = 1;                    // Mark as started
      } else {
          // Standard Toggle Logic
          if(NVIC_ST_CTRL_R & 0x01) {
              Music_Stop();     // Pause
          } else {
              Music_Resume();   // Resume
          }
      }
    }
    
    if(interrupted & 0x20){     // PC5 (Next)
      Music_Next();             //
    }
  }