// -------------------------------------------------------------
// File: esp8266.h
// Description: Header for merged FIFO-based ESP8266 driver
// -------------------------------------------------------------

#ifndef ESP8266_H
#define ESP8266_H

#include <stdint.h>

void ESP8266_Init(void);

void ESP8266_Reset(void);

void ESP8266_SetupWiFi(void);


void ESP8266_OutChar(char data);

void ESP8266_OutString(char *pt);

int ESP8266_GetMessage(char *datapt);

uint32_t ESP8266_AvailableInput(void);


#endif
