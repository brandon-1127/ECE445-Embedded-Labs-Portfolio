#include "Sound.h"
#include "Timer.h"
#include "../inc/tm4c123gh6pm.h"

#define PB1   (*((volatile uint32_t *)0x40005008))

void Sound_Init(void){
    // Initialize Port B for the speaker
    SYSCTL_RCGCGPIO_R |= 0x02;
    while((SYSCTL_PRGPIO_R&0x02) != 0x02){};
    
    GPIO_PORTB_DIR_R |= 0x02;      // 2) make PB1 output //dir is direction
    GPIO_PORTB_AFSEL_R &= ~0x02;
    GPIO_PORTB_DEN_R |= 0x02;      // 4) enable digital I/O on PB1 
    // Initialize Timer1A (440Hz = 80MHz / 440 / 2)
    Timer1A_Init(40000, 3); 
    TIMER1_CTL_R &= ~0x01; // Ensure it starts OFF
}

void Sound_AlarmStart(void){
    TIMER1_IMR_R |= 0x01;  // Arm Timer1A timeout interrupt
    TIMER1_CTL_R |= 0x01;  // Enable Timer1A
}

void Sound_AlarmStop(void){
    TIMER1_ICR_R = 0x01;   // Clear any pending interrupt flag
    TIMER1_IMR_R &= ~0x01; // Disarm Timer1A interrupt
    TIMER1_CTL_R &= ~0x01; // Disable Timer1A
    PB1 &= ~0x02;          // Force PB1 low (silence)
}

void Sound_Test(uint32_t frequency, uint32_t duration) {
    for(uint32_t i=0; i < duration; i++) {
        PB1 ^= 0x02;                     // Toggle pin
        
        // Manual delay for pitch
        for(uint32_t d=0; d < frequency; d++) {
            // Empty loop to stall CPU
        }
    }
    PB1 &= ~0x02; // Ensure speaker is OFF after test
}
