// Lab5.c
// Runs on TM4C123
// Spring 2025

/*
// *******Inputs from switches**************************
// bit1 PE1 Voice switch, positive logic switch, external pull down
// bit0 PE0 Play switch, positive logic switch, external pull down

// ********Outputs to DAC****************************
// TLV5616 SSIClk (SCLK) connected to PD0
// TLV5616 SSIFss (FS)   connected to PD1
// TLV5616 SSITx (DIN)   connected to PD3

// *****LCD graphics, and I/O with SDC***************
// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) connected to PA4 (SSIRx for SDC)
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) connected to PD7 (CS for SDC)
// Data/Command (pin 4) connected to PA6 (GPIO)
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

// Debugging, oscilloscope connected to 
// PF1 toggles in background
// PF3 toggles in foreground
*/

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "music.h"
#include "../inc/PLL.h"
#include "Switch.h"
#include "../inc/SysTickInts.h"
#include "../inc/TLV5616.h"
#include "../inc/ST7735.h"
#include "mailbox.h"
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode

extern const Song_t* Playlist[];
int32_t lastIndex = -1; // Initialize to a number that doesn't exist

int main(void){
  DisableInterrupts();
  PLL_Init(Bus80MHz);    // bus clock at 80 MHz
  ST7735_InitR(INITR_REDTAB); 
  // write this
  Music_Init();
  Switch_Init();
  EnableInterrupts();
while(1) {
    if (CurrentPlaylistIndex != lastIndex) {
        ST7735_FillScreen(0);   
        ST7735_DrawString(3, 3, Playlist[CurrentPlaylistIndex]->name, ST7735_WHITE);
        
        lastIndex = CurrentPlaylistIndex;
    }
}   
}
   
