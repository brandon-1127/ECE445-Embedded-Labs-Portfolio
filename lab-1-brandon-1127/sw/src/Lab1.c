// ----------------------------------------------------------------------------
// Lab1.c
// Jonathan Valvano
// July 8, 2024
// Possible main program to test the lab
// Feel free to edit this to match your specifications
#include <stdint.h>
#include "PLL.h"
#include "../inc/tm4c123gh6pm.h"

void Clock_Delay1ms(uint32_t n);
void LED_Init(void);
void LED_Out(uint32_t data);
void Switch_Init(void);
uint32_t Switch_In(void);

int main(void){
  PLL_Init(Bus80MHz); 
  LED_Init();
  Switch_Init();
  // Write something to declare required variables
  #define PE2   (*((volatile uint32_t *)0x40024010)) //port e base is 40024000 + 4*x04
  #define PC6   (*((volatile uint32_t *)0x40006100)) // port c base is 40006000 + 4*x40

  struct State{
    uint32_t output;
    uint32_t time;

    const struct State *Next[2];
  
  typedef const struct State States;

  #define R0 0
  #define T1 1
  #define R1 2
  #define T2 3
  #define R2 4
  #define T3 5
  #define R3 6
  #define T0 7

  // Write something to initalize the state of the FSM, LEDs, and variables as needed
  States FSM[8] = {
    {0, 10, { &FSM[R0], & FSM[T1]}},
    {0, 10, { &FSM[R1], & FSM[T1]}},
    {0, 10, { &FSM[R1], & FSM[T2]}},
    {0, 10, { &FSM[R2], & FSM[T2]}},
    {1, 10, { &FSM[R2], & FSM[T3]}},
    {1, 10, { &FSM[R3], & FSM[T3]}},
    {1, 10, { &FSM[R3], & FSM[T0]}},
    {1, 10, { &FSM[R0], & FSM[T0]}},
  };

  States *Pt = &FSM[0];

  while(1){
    // Write something using Switch_In() and LED_Out() to implement the behavior in the lab doc
    LED_Out(Pt->output);
    Clock_Delay1ms(Pt->time);
    uint32_t input = Switch_In();
    Pt = Pt->Next[input];

  }
}

void LED_Init(void){
    // Write something to initalize the GPIOs that drive the LEDs based on your EID as defined in the lab doc.
    // PE2

    SYSCTL_RCGCGPIO_R |= 0x10;     // 1) activate Port E
    while((SYSCTL_PRGPIO_R & 0x10)!=0x10){}; // wait to finish activating

    GPIO_PORTE_DEN_R |= 0x04; //enable digit i/o on PE2
    GPIO_PORTE_DIR_R |= 0x04; //enable output

    GPIO_PORTE_DATA_R |= 0x04; 
}
void LED_Out(uint32_t data){
    // write something that sets the state of the GPIO pin as required

    PE2 = (data << 2); //0x04 on, 0 is off
}
void Switch_Init(void){
    // write something to initalize the GPIO that take input from the switches based on your EID as defined in the lab doc
    //PC6
    SYSCTL_RCGCGPIO_R |= 0x04;     // 1) activate Port C
    while((SYSCTL_PRGPIO_R & 0x04)!=0x04){}; // wait to finish activating

    GPIO_PORTC_DEN_R |= 0x40; //enable digital i/o on PC6
    GPIO_PORTC_DIR_R &= ~0x40; //enable input

}

uint32_t Switch_In(void){
  // write something that reads the state of the GPIO pin as required
  return (PC6 >> 6);
}

void Clock_Delay(uint32_t ulCount){
  while(ulCount){
    ulCount--;
  }
}

// ------------Clock_Delay1ms------------
// Simple delay function which delays about n milliseconds.
// Inputs: n, number of msec to wait
// Outputs: none
void Clock_Delay1ms(uint32_t n){
  while(n){
    Clock_Delay(23746);  // 1 msec, tuned at 80 MHz, originally part of LCD module
    n--;
  }
}
