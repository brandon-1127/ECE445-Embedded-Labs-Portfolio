// ----------------------------------------------------------------------------
//
// File name: MQTT.c
//
// Description: This code is used to bridge the TM4C123 board and the MQTT Web Application
//              via the ESP8266 WiFi board

// Authors:       Mark McDermott
// Orig gen date: June 3, 2023
// Last update:   July 21, 2023
//
// ----------------------------------------------------------------------------


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "inc/tm4c123gh6pm.h"
#include "../inc/ST7735.h"
#include "../inc/PLL.h"

#include "UART.h"
#include "../inc/UART5.h"
#include "esp8266.h"
#include "Sound.h"

#define DEBUG1                // First level of Debug
//#undef  DEBUG1                // Comment out to enable Debug1

#define UART5_FR_TXFF            0x00000020  // UART Transmit FIFO Full
#define UART5_FR_RXFE            0x00000010  // UART Receive FIFO Empty
#define UART5_LCRH_WLEN_8        0x00000060  // 8 bit word length
#define UART5_LCRH_FEN           0x00000010  // UART Enable FIFOs
#define UART5_CTL_UARTEN         0x00000001  // UART Enable

#define CLOCK_MODE 0
#define SETTING_MODE 1
#define ALARM_MODE 2

#define SET_CLOCK 0 
#define SET_ALARM 1

// ----   Function Prototypes not defined in header files  ------
// 
void EnableInterrupts(void);    // Defined in startup.s

// ----------   VARIABLES  ----------------------------

extern volatile uint32_t Hours;
extern volatile uint32_t Minutes;
extern volatile uint32_t Seconds;
extern volatile uint32_t AlarmHours;
extern volatile uint32_t AlarmMinutes;
extern volatile uint32_t generalmode;
extern volatile uint32_t setting;
extern volatile int32_t temp;
extern volatile uint32_t is12Hour;

//Buffers for send / recv
char                    input_char;
char                    b2w_buf[64];

char                    w2b_buf[128];
static uint32_t         bufpos          = 0;

static uint32_t append_u32(char *dst, uint32_t idx, uint32_t max, uint32_t value){
    char tmp[10];
    uint32_t n = 0;

    if (value == 0) {
        if (idx < max) dst[idx++] = '0';
        return idx;
    }

    while (value > 0 && n < sizeof(tmp)) {
        tmp[n++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (n > 0 && idx < max) {
        dst[idx++] = tmp[--n];
    }
    return idx;
}

static uint32_t append_i32(char *dst, uint32_t idx, uint32_t max, int32_t value){
    if (value < 0) {
        if (idx < max) dst[idx++] = '-';
        // Convert safely without overflow on INT32_MIN
        uint32_t mag = (uint32_t)(-(value + 1)) + 1u;
        return append_u32(dst, idx, max, mag);
    }
    return append_u32(dst, idx, max, (uint32_t)value);
}

// --------------------------     W2B Parser      ------------------------------
//
// This parser decodes and executes commands from the MQTT Web Appication 
//
void Parser(void) {
    char *token = strtok(w2b_buf, ","); // Get the command string
    if (token == NULL) return;

    // Trim leading/trailing whitespace and CR/LF from UART/ESP framing
    while (*token && isspace((unsigned char)*token)) {
        token++;
    }
    size_t len = strlen(token);
    while (len > 0 && isspace((unsigned char)token[len - 1])) {
        token[len - 1] = '\0';
        len--;
    }

    if (strcmp(token, "incHour") == 0) {
        if ((generalmode == SETTING_MODE) && (setting == SET_ALARM)) {
            AlarmHours = (AlarmHours + 1) % 24;
        } else {
            Hours = (Hours + 1) % 24;
        }
    }
    else if (strcmp(token, "decHour") == 0) { 
        if ((generalmode == SETTING_MODE) && (setting == SET_ALARM)) {
            AlarmHours = (AlarmHours == 0) ? 23 : AlarmHours - 1;
        } else {
            Hours = (Hours == 0) ? 23 : Hours - 1;
        }
    }
    else if (strcmp(token, "incMin") == 0) { 
        if ((generalmode == SETTING_MODE) && (setting == SET_ALARM)) {
            AlarmMinutes = (AlarmMinutes + 1) % 60;
        } else {
            Minutes = (Minutes + 1) % 60;
        }
    }
    else if (strcmp(token, "decMin") == 0) { 
        if ((generalmode == SETTING_MODE) && (setting == SET_ALARM)) {
            AlarmMinutes = (AlarmMinutes == 0) ? 59 : AlarmMinutes - 1;
        } else {
            Minutes = (Minutes == 0) ? 59 : Minutes - 1;
        }
    }
    else if (strcmp(token, "reset") == 0) { 
        // Reset now acts as "silence alarm" without changing current time
        generalmode = CLOCK_MODE;
        Sound_AlarmStop();
    }
    else if (strcmp(token, "toggleMode") == 0) { 
        generalmode = (generalmode + 1) % 2; //toggle between clock and setting mode
    }
    else if (strcmp(token, "toggleSetting") == 0) { // Toggles between SET_CLOCK and SET_ALARM
        setting = (setting == SET_CLOCK) ? SET_ALARM : SET_CLOCK;   
    }
    else if (strcmp(token, "toggleTime") == 0){
        is12Hour = (is12Hour + 1) % 2;
    }
    else {
        #ifdef DEBUG1
        UART_OutString("Unknown Command: ");
        UART_OutString(token);
        UART_OutString("\r\n");
        #endif
    }

    #ifdef DEBUG1
        UART_OutString("Executed Command: ");
        UART_OutString(token);
        UART_OutString("\r\n");
    #endif
}
  
// -----------------------  TM4C_to_MQTT Web App -----------------------------
// This routine publishes clock data to the
// MQTT Web Application via the MQTT Broker
// The data is sent using CSV format:
//
// ----------------------------------------------------------------------------
//    
//    Convert this routine to use a FIFO
//
// 
/*
void TM4C_to_MQTT(void){
	
  sprintf(b2w_buf, "%d,%d,%d,%d,%d,%d,%d\n", Hours, Minutes, Seconds, generalmode, setting, temp, is12Hour);           
  
  ESP8266_OutString(b2w_buf);      

  #ifdef DEBUG1
   UART_OutString("B2W: ");
   UART_OutString(b2w_buf);         
   UART_OutString("\r\n"); 
  #endif
}
*/
void TM4C_to_MQTT(void){
    uint32_t i = 0;
    uint32_t max = sizeof(b2w_buf) - 1; // keep room for '\0'

    i = append_u32(b2w_buf, i, max, Hours);
    if (i < max) b2w_buf[i++] = ',';
    i = append_u32(b2w_buf, i, max, Minutes);
    if (i < max) b2w_buf[i++] = ',';
    i = append_u32(b2w_buf, i, max, Seconds);
    if (i < max) b2w_buf[i++] = ',';
    i = append_u32(b2w_buf, i, max, generalmode);
    if (i < max) b2w_buf[i++] = ',';
    i = append_u32(b2w_buf, i, max, setting);
    if (i < max) b2w_buf[i++] = ',';
    i = append_i32(b2w_buf, i, max, temp);
    if (i < max) b2w_buf[i++] = ',';
    i = append_u32(b2w_buf, i, max, is12Hour);
    if (i < max) b2w_buf[i++] = '\n';
    b2w_buf[i] = '\0';
  
  ESP8266_OutString(b2w_buf);      

    // NOTE: TM4C_to_MQTT runs in a timer ISR context.
    // Avoid UART debug printing here because it is blocking and can destabilize timing.
}
 
// -------------------------   MQTT_to_TM4C  -----------------------------------
// This routine receives the command data from the MQTT Web App and parses the
// data and feeds the commands to the TM4C.
// -----------------------------------------------------------------------------
//
//    Convert this routine to use a FIFO
//
// 
void MQTT_to_TM4C(void) {
    while (ESP8266_GetMessage(w2b_buf)) {
        Parser();
    }
}   // End of routine

