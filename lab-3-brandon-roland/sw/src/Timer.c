// Timer0A.c

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"

#define NVIC_EN0_INT19          0x00080000  // Interrupt 19 enable
#define TIMER_CFG_16_BIT        0x00000004  // 16-bit timer configuration,
                                            // function is controlled by bits
                                            // 1:0 of GPTMTAMR and GPTMTBMR
#define TIMER_TAMR_TACDIR       0x00000010  // GPTM Timer A Count Direction
#define TIMER_TAMR_TAMR_PERIOD  0x00000002  // Periodic Timer mode
#define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
#define TIMER_IMR_TATOIM        0x00000001  // GPTM TimerA Time-Out Interrupt
                                            // Mask
#define TIMER_ICR_TATOCINT      0x00000001  // GPTM TimerA Time-Out Raw
                                            // Interrupt
#define TIMER_TAILR_TAILRL_M    0x0000FFFF  // GPTM TimerA Interval Load
                                            // Register Low


void (*PeriodicTask0)(void);   // user function

// ***************** Timer0A_Init ****************
// Activate Timer0A interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in 12.5ns units
//          priority 0 (highest) to 7 (lowest)
// Outputs: none
void Timer0A_Init(void(*task)(void), uint32_t period, uint32_t priority){
  SYSCTL_RCGCTIMER_R |= 0x01;      // 0) activate timer0
  PeriodicTask0 = task;            // user function (this line also allows time to finish activating)
  TIMER0_CTL_R &= ~0x00000001;     // 1) disable timer0A during setup
  TIMER0_CFG_R = 0x00000000;       // 2) configure for 32-bit timer mode
  TIMER0_TAMR_R = 0x00000002;      // 3) configure for periodic mode, default down-count settings
  TIMER0_TAILR_R = period-1;       // 4) reload value
  TIMER0_TAPR_R = 0;               // 5) 12.5ns timer0A
  TIMER0_ICR_R = 0x00000001;       // 6) clear timer0A timeout flag
  TIMER0_IMR_R |= 0x00000001;      // 7) arm timeout interrupt
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|(priority<<29); 
  NVIC_EN0_R = NVIC_EN0_INT19;     // 9) enable interrupt 19 in NVIC
  TIMER0_CTL_R |= 0x00000001;      // 10) enable timer0A
}

void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer0A timeout
  (*PeriodicTask0)();                // execute user task
}
void Timer0A_Stop(void){
  NVIC_EN0_R = 1<<19;            // 9) disable interrupt 19 in NVIC
  TIMER0_CTL_R = 0x00000000;     // 10) disable timer0A
}

#define PB1   (*((volatile uint32_t *)0x40005008))

//Timer1
void Timer1A_Init(uint32_t period, uint32_t priority){
  SYSCTL_RCGCTIMER_R |= 0x02;      // Activate timer1
  TIMER1_CTL_R &= ~0x00000001;     // Disable timer1A during setup
  TIMER1_CFG_R = 0x00000000;       // 32-bit mode
  TIMER1_TAMR_R = 0x00000002;      // Periodic mode
  TIMER1_TAILR_R = period-1;       // Reload value
  TIMER1_ICR_R = 0x00000001;       // Clear timeout flag
  TIMER1_IMR_R |= 0x00000001;      // Arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|(priority<<13); // Timer1A is Int 21
  NVIC_EN0_R = 1<<21;              // Enable interrupt 21
}

// Extern declarations for sine wave
extern const uint8_t sine_table[50];
extern uint32_t sine_index;
extern uint32_t sample_counter;
extern const uint32_t samples_per_tick;

void Timer1A_Handler(void){
  TIMER1_ICR_R = 0x01;             // Acknowledge
  PB1 ^= 0x02;       // Toggle PB1 (Speaker)
}

