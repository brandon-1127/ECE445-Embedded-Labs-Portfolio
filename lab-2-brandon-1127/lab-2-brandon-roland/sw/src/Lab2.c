// ----------------------------------------------------------------------
//
// Lab2.c
// Runs on TM4C123
// 
// Jonathan Valvano
// December 27, 2024
//
//  Three possible ADC inputs are PE4, PE5, or PE1.      
// ----------------------------------------------------------------------

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ADCSWTrigger.h"
#include "../inc/LaunchPad.h"
#include "../inc/PLL.h"
#include "../inc/Timer2A.h"
#include "../inc/CortexM.h"
#include "../inc/UART.h"
#include "../inc/SSD1306.h"
// bit-specific addresses for PC7 and PC6
#define PC7   (*((volatile uint32_t *)0x40006200))
#define PC6   (*((volatile uint32_t *)0x40006100))
void ADC_Init(void){
  SYSCTL_RCGCADC_R |= 0x0001;   // 7) activate ADC0 turns on clock
                                  // 1) activate clock for Port E
  SYSCTL_RCGCGPIO_R |= 0x10;
  while((SYSCTL_PRGPIO_R&0x10) != 0x10){};

  GPIO_PORTE_DIR_R &= ~0x20;      // 2) make PE5 input //dir is direction, & means clearing so input
  GPIO_PORTE_AFSEL_R |= 0x20;     // 3) enable alternate function on PE5 (ADC instead of GPIO)
  GPIO_PORTE_DEN_R &= ~0x20;      // 4) disable digital I/O on PE5 //disable reading digital voltage
  GPIO_PORTE_AMSEL_R |= 0x20;     // 5) enable analog functionality on PE5 //connect it to ADC
    
  while((SYSCTL_PRADC_R&0x0001) != 0x0001){};    // good code, but not yet implemented in simulator

  //i guess some basic ADC settings to configure
  ADC0_PC_R &= ~0xF;              // 7) clear max sample rate field
  ADC0_PC_R |= 0x1;               //    maximum speed is 125K samples/sec
  ADC0_SSPRI_R = 0x0123;          // 8) Sequencer 3 is highest priority
  ADC0_ACTSS_R &= ~0x0008;        // 9) disable sample sequencer 3
  ADC0_EMUX_R &= ~0xF000;         // 10) seq3 is software trigger //starts only when told
  ADC0_SSMUX3_R &= ~0x000F;       // 11) clear SS3 field
  ADC0_SSMUX3_R += 8;             //    set channel // mapped to PE5
  ADC0_SSCTL3_R = 0x0006;         // 12) no TS0 D0, yes IE0 END0
  ADC0_IM_R &= ~0x0008;           // 13) disable SS3 interrupts
  ADC0_ACTSS_R |= 0x0008;         // 14) enable sample sequencer 3
}

int32_t ADC_In(void){
  uint32_t result;
  ADC0_PSSI_R = 0x0008;            // 1) initiate SS3
  while((ADC0_RIS_R&0x08)==0){};   // 2) wait for conversion done //busy-wait
    // if you have an A0-A3 revision number, you need to add an 8 usec wait here
  result = ADC0_SSFIFO3_R&0xFFF;   // 3) read result the sequener fifo bottom 3 bytes
  ADC0_ISC_R = 0x0008;             // 4) acknowledge completion 

  return result;
}

// debugging dump variables
#define BUFSIZE 1000
uint32_t TimeBuf[BUFSIZE]; // in bus cycles
uint32_t DataBuf[BUFSIZE]; // 0 to 4095 assuming constant analog input
volatile uint32_t Num;     // index from 0 to BUFSIZE-1

// time jitter variables
uint32_t MinT;    // minimum(TimeBuf[i-1] � TimeBuf[i]) for i equals 1 to BUFSIZE-1
uint32_t MaxT;    // maximum(TimeBuf[i-1] � TimeBuf[i]) for i equals 1 to BUFSIZE-1
uint32_t Jitter;  // MaxT � MinT (in bus cycles)
uint16_t Periods[256];  // histogram of times between ADC triggers, optional

// SNR variables
uint32_t Averaging; // 1,2,4,8,16,32, or 64 to student CLT
uint32_t Vmin, Vmax, PMFmax;
int32_t Signal,Noise,SNR,Distance,ENOB;
uint16_t PMF[100];  // histogram of ADC samples
uint32_t adc0_sac_r_test;


// Samples ADC at 1000 Hz
void RealTimeTask(void){uint32_t ADCvalue;
  PC7 ^= 0x80;  // profile
  PC7 ^= 0x80;  // profile
  ADCvalue = ADC_In();
  if(Num < BUFSIZE){
    TimeBuf[Num] = TIMER1_TAR_R;
    DataBuf[Num] = ADCvalue;
    Num++;
  }
  PC7 ^= 0x80;  // profile
}

void CalculateJitter(void){
// MaxT = maximum(Time[i-1] � Time[i]) for i equals 1 to 999
// MinT = minimum(Time[i-1] � Time[i]) for i equals 1 to 999
// Jitter = MaxT � MinT 
// write this
  MaxT = TimeBuf[0] - TimeBuf[1];
  MinT = TimeBuf[0] - TimeBuf[1];
  for(int i = 2; i < BUFSIZE; i++){
    if((TimeBuf[i - 1] - TimeBuf[i]) > MaxT) MaxT = (TimeBuf[i - 1] - TimeBuf[i]);
    if((TimeBuf[i - 1] - TimeBuf[i]) < MinT) MinT = (TimeBuf[i - 1] - TimeBuf[i]);
  }
  Jitter = MaxT - MinT;
}

uint32_t sqrt2(uint32_t s){ int n; // loop counter
uint32_t t;            // t*t will become s
  t = s/16+1;          // initial guess
  for(n = 16; n; --n){ // will finish
    t = ((t*t+s)/t)/2;
  }
  return t;
}

void CalculateSNR(void){
  // signal is the average of BUFSIZE ADC samples
  // noise is the standard deviation of the BUFSIZE ADC samples
  // SNR = signal/noisem(no floating point allowed)
    // write this
    ENOB = 0;
    uint32_t total = 0;
    for(int i = 0; i < BUFSIZE; i++){
      total += DataBuf[i];
    }
    Signal = total/BUFSIZE;

    total = 0;
    int32_t diff = 0;
    for(int i = 0; i < BUFSIZE; i++){
      diff = (int32_t)DataBuf[i] - (int32_t)Signal;
      total += diff*diff;
    }
    Noise = sqrt2(total/BUFSIZE);

    if(Noise == 0) SNR = 10000;
    else SNR = Signal/Noise;
    
    uint32_t temp = SNR;
    while(temp > 1){
      temp = temp >> 1;
      ENOB++;
    }
}

void CreatePMF(void){
  int i;
  Vmin = 0xFFFFFFFF; Vmax = 0;
  for(i=0; i<BUFSIZE; i++){uint32_t v;
    v = DataBuf[i];
    if(v>Vmax) Vmax = v;
    if(v<Vmin) Vmin = v;
  }    
  for(i=0; i<100; i++){
    PMF[i] = 0;
  }    
  if(Vmin>25) Vmin = Vmin-25;   // room     
  PMFmax = 0;       
  for(i=0; i<BUFSIZE; i++) {
    uint32_t v;
    v = DataBuf[i];
    if((v>=Vmin)&&(v<(Vmin+100))){
      PMF[v-Vmin]++;
      if(PMF[v-Vmin]>PMFmax) PMFmax =PMF[v-Vmin];
    }        
  }
  UART_OutString("Averaging =");UART_OutUDec(Averaging); UART_OutChar(CR); UART_OutChar(LF);
  for(i=0; i<100; i++){
    UART_OutUDec(Vmin+i); UART_OutChar(',');UART_OutUDec(PMF[i]); 
    UART_OutChar(CR); UART_OutChar(LF);
  }
}
  
// initialize PC6 and PC7 as GPIO outputs
// warning: this code must be friendly because the debugger is on PC3-0
void PortC_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x00000004;     // activate clock for Port C
  while((SYSCTL_PRGPIO_R&0x04) == 0){};// allow time for clock to stabilize
  GPIO_PORTC_PCTL_R = GPIO_PORTC_PCTL_R&0x00FFFFFF;  // PC6-7 GPIO
  GPIO_PORTC_DIR_R |= 0xC0;            // PC6-7 out
  GPIO_PORTC_DEN_R |= 0xC0;            // enable digital I/O on PC6-7
  GPIO_PORTC_DATA_R &= ~0xC0;          // off
}

// -----------          Timer1_Init           ---------------------------
// Reading TIMER1_TAR_R will return the 32-bit current time in 12.5ns units. 
// The timer counts down. 
void Timer1_Init(void){
  volatile uint32_t delay;
  SYSCTL_RCGCTIMER_R    |= 0x02;                // 0) activate TIMER1a
  delay                  = SYSCTL_RCGCTIMER_R;  // allow time to finish activating
  TIMER1_CTL_R           = 0x00000000;          // 1) disable TIMER1A during setup
  TIMER1_CFG_R           = 0x00000000;          // 2) configure for 32-bit mode
  TIMER1_TAMR_R          = 0x00000002;          // 3) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R         = 0xFFFFFFFF;          // 4) reload value
  TIMER1_TAPR_R          = 0;                   // 5) bus clock resolution
  TIMER1_CTL_R           = 0x00000001;          // 10) enable TIMER1A
}


// d = A/(ADCvalue+B);
// short range IR GP2Y0A41SK0F
//#define A 264336
//#define B -175
//#define IRmax 2552
//#define Dmax 400
// long range IR GP2Y0A21YK0F
// ***Calibrate your sensor, DO NOT USE THESE VALUES***
#define A 249883
#define B -221
#define IRmax 900
#define Dmax 400
int32_t IR_Convert(int32_t n){  // returns distance in mm
    // write this
    
    if((n + B) == 0) return 0;
    
    Distance = A/(n + B);

    return Distance;

}

int32_t InvokesDivide=10000; 
// The purpose of InvokesDivide is to invoke a divide instruction
// Otherwise, InvokesDivide has no functional purpose

// ----------------------------------------------------------------------
// -------------------    MAIN without SSD1306  ----------------------------------------
// ----------------------------------------------------------------------
// Use debugger to observe results
// PMF output to serial port


int main(void){ 
  DisableInterrupts(); 
  PLL_Init(Bus80MHz);                // 80 MHz
  LaunchPad_Init();
  Timer1_Init();
  PortC_Init();
  UART_Init();  // to print the PMF
  Timer2A_Init(&RealTimeTask,80000,1); // 1kHz, priority=1
  ADC_Init();           // sample PE5
  EnableInterrupts(); 
  while(1){
    ADC0_SAC_R = 0; // this will hard fault if ADC_Init is not complete
    for(Averaging = 1; Averaging<=64; ){
      Num = 0;  // empty array
      while(Num<1000){
        InvokesDivide = (InvokesDivide*12345678)/12345; // this line should have caused jitter
        PC6 ^= 0x40;    // toggles PC6 when running in main, this line has a critical section
        
      }
      CalculateJitter();
      CalculateSNR();
      CreatePMF();
      Distance = IR_Convert(Signal);

      if(PF4==0x00){                 // change SAR size on button push
        Clock_Delay1ms(10);          // debounce
        while(PF4==0x00){
          Clock_Delay1ms(10);
        }
        Averaging = Averaging<<1;
        ADC0_SAC_R = ADC0_SAC_R+1;
        adc0_sac_r_test = ADC0_SAC_R;

      } 
    }
  }
}

// ----------------------------------------------------------------------
// -------------------    MAIN with SSD1306  ----------------------------------------
// ----------------------------------------------------------------------
// PMF output to serial port
int main1(void){ 
  DisableInterrupts(); 
  PLL_Init(Bus80MHz);                // 80 MHz
  LaunchPad_Init();
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  SSD1306_ClearBuffer();
  SSD1306_OutBuffer();
  SSD1306_SetCursor(0,0);
  SSD1306_OutString("Lab 2");
  SSD1306_SetCursor(0,1);
  SSD1306_OutString("SAC =");

  SSD1306_SetCursor(0,2);
  SSD1306_OutString("ADC =");
  SSD1306_SetCursor(0,3);
  SSD1306_OutString("Dis =");
  Timer1_Init();
  PortC_Init();
  UART_Init();  // to print the PMF
  Timer2A_Init(&RealTimeTask,80000,1); // 1kHz, priority=1
  ADC_Init();           // sample PE5
  SSD1306_SetCursor(6,1);
  SSD1306_OutUDec(ADC0_SAC_R);
  EnableInterrupts(); 
  while(1){
    ADC0_SAC_R = 0; 
    for(Averaging = 1; Averaging<=64; ){
      Num = 0;  // empty array
      while(Num<1000){
        InvokesDivide = (InvokesDivide*12345678)/12345; // this line should have caused jitter
        GPIO_PORTC_DATA_R ^= 0x40;    // toggles PC6 when running in main, this line has a critical section
      }
      CalculateJitter();
      CalculateSNR();
      CreatePMF();
      Distance = IR_Convert(Signal);
      SSD1306_SetCursor(6,2);
      SSD1306_OutUDec(Signal);
      SSD1306_SetCursor(6,3);
      SSD1306_OutUDec(Distance);

      if(PF4==0x00){                 // change SAR size on button push
        Clock_Delay1ms(10);          // debounce
        while(PF4==0x00){
          Clock_Delay1ms(10);
        }
        Averaging = Averaging<<1;
        ADC0_SAC_R = ADC0_SAC_R+1;
        SSD1306_SetCursor(6,1);
        SSD1306_OutUDec(ADC0_SAC_R);

      } 
    }
  }
}
