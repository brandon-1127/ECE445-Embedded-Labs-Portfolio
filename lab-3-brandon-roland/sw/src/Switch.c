#include "Switch.h"
#include "Sound.h"
#include "../inc/tm4c123gh6pm.h"

#define CLOCK_MODE 0
#define SETTING_MODE 1
#define ALARM_MODE 2

#define SET_CLOCK 0 
#define SET_ALARM 1

// Debounce timing: 5ms debounce delay
#define DEBOUNCE_TIME 400000  // 5ms at 80MHz clock


extern volatile uint32_t generalmode;
extern volatile uint32_t Hours;
extern volatile uint32_t Minutes;
extern volatile uint32_t AlarmHours;
extern volatile uint32_t AlarmMinutes;
extern uint32_t setting;
extern uint32_t MHindex;

// Debounce helper function - simple busy-wait delay
static void Debounce_Delay(void) {
  volatile uint32_t delay = DEBOUNCE_TIME;
  while(delay--) {}
}

// Debounce helper - verify button state is stable
// Returns 1 if button is still pressed, 0 if it bounced
static uint32_t Debounce_VerifyPortC(uint32_t pins) {
  Debounce_Delay();
  return (GPIO_PORTC_DATA_R & pins);  // Returns 0 if pressed (active low)
}

static uint32_t Debounce_VerifyPortF(uint32_t pins) {
  Debounce_Delay();
  return (GPIO_PORTF_DATA_R & pins);  // Returns 0 if pressed (active low)
}

void Switch_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x24;     // Activate Port C and Port F
  while((SYSCTL_PRGPIO_R & 0x24) != 0x24){};

  // Port C Setup (External Buttons PC4-7)
  GPIO_PORTC_DIR_R &= ~0xF0;     //input
  GPIO_PORTC_AFSEL_R &= ~0xF0;   //disable other functions
  GPIO_PORTC_PUR_R |= 0xF0;      //pull up res
  GPIO_PORTC_DEN_R |= 0xF0;      //enable digital
  
  // Port C Interrupt Configuration
  GPIO_PORTC_IS_R &= ~0xF0;      // Edge-sensitive
  GPIO_PORTC_IBE_R &= ~0xF0;     // Not both edges
  GPIO_PORTC_IEV_R &= ~0xF0;     // Falling edge (pressed)
  GPIO_PORTC_ICR_R = 0xF0;       // Clear flags
  GPIO_PORTC_IM_R |= 0xF0;       // Arm interrupts
  
  // Port F Setup (Built-in Button PF4)
  GPIO_PORTF_LOCK_R = 0x4C4F434B; //unlock key
  GPIO_PORTF_CR_R |= 0x10; 
  GPIO_PORTF_DIR_R &= ~0x10;     //input
  GPIO_PORTF_AFSEL_R &= ~0x10;   //disable other functions
  GPIO_PORTF_PUR_R |= 0x10;      //pull up res
  GPIO_PORTF_DEN_R |= 0x10;      //enable digital

  /* 
  GPIO_PORTF_LOCK_R = 0x4C4F434B; //unlock key
  GPIO_PORTF_CR_R |= 0x10;        //allow changes to pf4
  GPIO_PORTF_DIR_R &= ~0x10;      //input
  GPIO_PORTF_PUR_R |= 0x10;       //enable PU res
  GPIO_PORTF_DEN_R |= 0x10;       //enable digital
  */
  
  // Port F Interrupt Configuration
  GPIO_PORTF_IS_R &= ~0x10;      
  GPIO_PORTF_IBE_R &= ~0x10;     
  GPIO_PORTF_IEV_R &= ~0x10;     
  GPIO_PORTF_ICR_R = 0x10;       
  GPIO_PORTF_IM_R |= 0x10;       

  // NVIC Configuration (Priority and Enable)
  // Port C is Interrupt 2, Port F is Interrupt 30
  NVIC_PRI0_R = (NVIC_PRI0_R&0xFF00FFFF)|0x00A00000; // Port C Priority 5
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // Port F Priority 5
  NVIC_EN0_R = 0x40000004;       // Enable Interrupt 2 (C) and 30 (F)
}

void GPIOPortC_Handler(void){ //portc button ISR
  uint32_t interrupted = GPIO_PORTC_RIS_R; // Read hardware flags
  GPIO_PORTC_ICR_R = 0xF0;                // Acknowledge all four PC4-7
  
  // Debounce: verify button is still pressed after delay
  if(Debounce_VerifyPortC(interrupted & 0xF0)) {
    return;  // Button bounced, ignore
  }
  
  // Any button stops alarm
  if(generalmode == ALARM_MODE){
    generalmode = CLOCK_MODE;
    Sound_AlarmStop();
    return;
  }

  if(generalmode == SETTING_MODE){
    if(interrupted & 0x20) setting = (setting + 1) % 2; // Toggle Time vs Alarm
    if(interrupted & 0x40) MHindex = (MHindex + 1) % 2;           // Toggle Hours vs Minutes

    volatile uint32_t *pt; // Pointer to the variable we want to change
    uint32_t max;          // The rollover point (24 or 60)

    // Determine target variable
    if(setting == 0){ // Editing Live Time
      pt = (MHindex == 0) ? &Hours : &Minutes;
    } else {                // Editing Alarm
      pt = (MHindex == 0) ? &AlarmHours : &AlarmMinutes;
    }
    max = (MHindex == 0) ? 24 : 60;

    // update
    if(interrupted & 0x10){ // UP
      *pt = (*pt + 1) % max;
    }
    if(interrupted & 0x80){ // DOWN
      if(*pt == 0) *pt = max - 1;
      else *pt = *pt - 1;
    }
  }

  if(generalmode != ALARM_MODE){ // Don't beep if we are already stopping the alarm 
    Sound_AlarmStart(); 
    for(volatile uint32_t i=0; i<80000; i++); // Short 10ms beep at 80MHz 
    Sound_AlarmStop(); 
  }
}

void GPIOPortF_Handler(void){ //portf button ISR
  GPIO_PORTF_ICR_R = 0x10; // Acknowledge PF4
  
  // Debounce: verify button is still pressed after delay
  if(Debounce_VerifyPortF(0x10)) {
    return;  // Button bounced, ignore
  }
  
  if(generalmode == ALARM_MODE){
    generalmode = CLOCK_MODE;
    Sound_AlarmStop();
  } else {
    generalmode = (generalmode + 1) % 2; 
    Sound_AlarmStart(); 
    for(volatile uint32_t i=0; i<80000; i++); // Short 10ms beep at 80MHz 
    Sound_AlarmStop(); 
  }
}

