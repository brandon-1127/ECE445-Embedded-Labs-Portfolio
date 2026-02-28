// -------------------------------------------------------------------
// File name:     Lab4E_Main.c
// Description:   This code is the main loop for the new MQTT Clock Control IOT Lab
//               
// Authors:       Mark McDermott
// Date:          June 6, 2023
//
// 

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "inc/tm4c123gh6pm.h"
#include "inc/ST7735.h"
#include "inc/PLL.h"
#include "Timer.h"
#include "inc/UART.h"
#include "inc/UART5.h"
#include "esp8266.h"
#include "MQTT.h"
#include "inc/Unified_Port_Init.h"

#include "Timer.h"
#include "Switch.h"
#include "Sound.h"
#include "NTC_Thermistor.h"

//#include "Lab4E.h"

uint32_t         Mode_Value;      //
uint32_t         Left_Value;      //
uint32_t         Right_Value;     //
uint32_t         Up_Value;        //
uint32_t         Down_Value;      //

#define CLOCK_MODE 0
#define SETTING_MODE 1
#define ALARM_MODE 2

#define SET_CLOCK 0 
#define SET_ALARM 1

volatile uint32_t Seconds = 0, Minutes = 0, Hours = 0;
volatile uint32_t AlarmHours = 0, AlarmMinutes = 0;
volatile uint32_t generalmode = 0, setting = 0, MHindex = 0;
uint32_t lastsecond = 68;
uint32_t lasthour = 25, lastminute = 61;
uint32_t lastmode = 0, lastsetting = 0, lastMHindex = 0;
uint32_t tempUpdateCounter = 0;  // Counter for 100ms temperature updates
volatile int32_t temp = 0;
volatile uint32_t is12Hour = 0; // 0 = 24hr mode, 1 = 12hr mode


//----- Prototypes of functions in startup.s  ----------------------
//
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // Go into low power mode

void Clocking(void);

void pause(void){
	while((PF0==0x01)&&(PF4==0x10)){
		for(int32_t i=0; i<0x80000; i++){
		}
	}
	while(!((PF0==0x01)&&(PF4==0x10))){
		for(int32_t i=0; i<0x80000; i++){
		}
	}
}

// -----------------------------------------------------------------
// -------------------- MAIN LOOP ----------------------------------
//
int main(void){  
  DisableInterrupts();            // Disable interrupts until finished with inits     
  PLL_Init(Bus80MHz);             // Bus clock at 80 MHz
  UART_Init();                    // Allow us to talk to the PC via PuTTy!          
  ST7735_InitR(INITR_REDTAB);     // Start up display.
  Unified_Port_Init();						// Initialize the Ports used for this lab
	
  ST7735_SetCursor(0,0);
  ST7735_OutString("Reseting ESP\n");
  ESP8266_Init();
  ESP8266_Reset();                   // Reset the WiFi module
  ESP8266_SetupWiFi();                    // Setup communications to MQTT Broker via 8266 WiFi
  ST7735_FillScreen(ST7735_BLACK);       // Clear setup text once ESP/MQTT is ready
  Switch_Init();
  Sound_Init();
  NTC_Init();
  Timer0A_Init(&Clocking, 80000000, 2);
  Timer2A_Init(&MQTT_to_TM4C, 400000, 7);         // Check/Get data from the ESP every 5ms 
  Timer5A_Init(&TM4C_to_MQTT, 8000000, 7);        // Send data back to MQTT Web App every 100ms
  
  EnableInterrupts();

  //Integrate your lab 3 here
  while(1){ 
	tempUpdateCounter++;  // Increment counter for temperature updates
    
    uint32_t currHour = Hours;
    uint32_t currMinute = Minutes;
    uint32_t currSecond = Seconds;

	if(generalmode == CLOCK_MODE){
      Draw_Clock(currHour, currMinute, currSecond, generalmode, 0, 0);
    } else if(generalmode == SETTING_MODE){
      if(setting == SET_CLOCK){
        Draw_Clock(currHour, currMinute, currSecond, generalmode, SET_CLOCK, MHindex);
      } else {
        Draw_Clock(AlarmHours, AlarmMinutes, 0, generalmode, SET_ALARM, MHindex);
      }
    } else if(generalmode == ALARM_MODE){
      Draw_Clock(currHour, currMinute, currSecond, ALARM_MODE, 0, 0);
    }

    if(currHour != lasthour || currMinute != lastminute || currSecond != lastsecond || generalmode != lastmode || setting != lastsetting || MHindex != lastMHindex){ 
      lasthour = currHour;
      lastminute = currMinute;
      lastsecond = currSecond;
      lastmode = generalmode;
      lastsetting = setting;
      lastMHindex = MHindex;

      Draw_AnalogClock(currHour, currMinute, currSecond);
    }

    // Update temperature display every 100ms
    if(tempUpdateCounter >= 50) {
      Draw_Temperature();
      tempUpdateCounter = 0;
    }

		/*
		ST7735_SetCursor(0,11);
		ST7735_OutString("Mode is now:    ");
		ST7735_SetCursor(13,11);
		ST7735_OutUDec( Mode_Value );
		Mode_Value ^= 0x01;
		*/
  }
}

void Clocking(){ //function caled by ISR
  Seconds++;
  if(Seconds == 60){
    Minutes++;
    Seconds = 0;
    if(Minutes == 60){
    Minutes = 0;
    Hours = (Hours + 1)%24;
    }
  }
  // Check for Alarm Trigger
  if((Hours == AlarmHours) && (Minutes == AlarmMinutes) && (Seconds == 0)){
    generalmode = ALARM_MODE;
    Sound_AlarmStart();
  }
}

void Draw_Clock(uint32_t hours, uint32_t minutes, uint32_t seconds, uint32_t genmode, uint32_t settingmode, uint32_t index_min_hour){//draws the clock

  ST7735_SetCursor(6, 3);

  uint16_t hColor = ST7735_WHITE;
  uint16_t mColor = ST7735_WHITE;
  uint16_t secColor = ST7735_WHITE;

  if (genmode == SETTING_MODE) {
    // In setting mode, color all digits (HH:MM:SS) by context
    if (settingmode == SET_CLOCK) {
      hColor = mColor = secColor = ST7735_YELLOW;
    } else {
      hColor = mColor = secColor = ST7735_CYAN;
    }
  } else if (genmode == ALARM_MODE) {
    // Flash everything RED
    hColor = mColor = secColor = ST7735_RED;
  }

  ST7735_SetTextColor(hColor);
  Display_Digits(hours);
  ST7735_SetTextColor(ST7735_WHITE); // Keep colons white
  ST7735_OutChar(':');
  ST7735_SetTextColor(mColor);
  Display_Digits(minutes);
  ST7735_SetTextColor(ST7735_WHITE);
  ST7735_OutChar(':');
  ST7735_SetTextColor(secColor);
  Display_Digits(seconds);
}

void Display_Digits(uint32_t digits){ //display helper function
  if(digits < 10){
    ST7735_OutChar('0');
    ST7735_OutChar(digits + '0');
  }else{
    ST7735_OutChar((digits/10) + '0');
    ST7735_OutChar((digits%10) + '0');
  }
}


void Draw_Temperature(void) { // Display temperature reading from NTC thermistor
  ST7735_SetCursor(3, 5);  // Position below the clock
  ST7735_SetTextColor(ST7735_GREEN);
  ST7735_OutString("Temp: ");
  
  temp = NTC_GetTemp();
  
  // temp is in units of 0.1°F
  int32_t whole = temp / 10;
  int32_t decimal = temp % 10;
  
  // Handle negative temperatures
  if (temp < 0 && decimal != 0) {
    decimal = -decimal;
  }
  
  // Display temperature with 2-digit format (XX.X°F)
  if (whole >= 0) {
    if (whole < 10) ST7735_OutChar('0');  // Pad with leading zero for single digits
    Display_Digits(whole);
  } else {
    ST7735_OutChar('-');
    if (whole > -10) ST7735_OutChar('0');  // Pad with leading zero for single digit negatives
    Display_Digits(-whole);
  }
  ST7735_OutChar('.');
  ST7735_OutChar('0' + (decimal % 10));  // Display only the decimal digit
  ST7735_OutChar(0xF8);  // Degree symbol
  ST7735_OutChar('F');
}

// Analog Clock Drawing Functions
// Clock center and dimensions
#define CLOCK_CENTER_X 64
#define CLOCK_CENTER_Y 115
#define CLOCK_RADIUS 35
#define HOUR_HAND_LEN 18
#define MINUTE_HAND_LEN 27
#define SECOND_HAND_LEN 32

// Convert angle (degrees) to x,y coordinates
// angle: 0-359 degrees (0 = 12 o'clock, 90 = 3 o'clock)
static void Angle_To_Coords(uint16_t angle, uint16_t length, int16_t *x, int16_t *y) {
  // Convert to radians and adjust so 0° is at top (12 o'clock)
  double radians = (angle - 90) * 3.14159 / 180.0;
  *x = CLOCK_CENTER_X + (int16_t)(length * cos(radians));
  *y = CLOCK_CENTER_Y + (int16_t)(length * sin(radians));
}

// Draw the clock face (circle and hour markers)
void Draw_AnalogClockFace(void) {
  static const char *hourLabels[12] = {
    "12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"
  };

  // Draw clock circle
  // ST7735_DrawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, ST7735_WHITE);
  
  // Draw hour markers (12 dots around the circle)
  for(uint16_t i = 0; i < 12; i++) {
    uint16_t angle = i * 30;  // 360 / 12 = 30 degrees per hour
    int16_t x, y;
    Angle_To_Coords(angle, CLOCK_RADIUS - 3, &x, &y);
    ST7735_DrawPixel(x, y, ST7735_WHITE);

    // Draw hour number slightly outside marker
    int16_t nx, ny;
    Angle_To_Coords(angle, CLOCK_RADIUS + 4, &nx, &ny);

    // Convert desired pixel location to text-grid location for DrawString
    int16_t xpix = nx - ((i == 0 || i >= 10) ? 6 : 3);  // center 2-char vs 1-char labels
    int16_t ypix = ny - 5;

    // Fine-tune a few labels that appear slightly high
    if(i == 1 || i == 4 || i == 8 || i == 11) {
      ypix += 3;
    }

    if(xpix < 0) xpix = 0;
    if(ypix < 0) ypix = 0;
    if(xpix > 127) xpix = 127;
    if(ypix > 159) ypix = 159;

    ST7735_DrawString((uint16_t)(xpix / 6), (uint16_t)(ypix / 10), (char *)hourLabels[i], ST7735_WHITE);
  }
  
  // Draw center dot
  ST7735_DrawPixel(CLOCK_CENTER_X, CLOCK_CENTER_Y, ST7735_WHITE);
}

// Draw clock hands based on current time
void Draw_AnalogClockHands(uint32_t hours, uint32_t minutes, uint32_t seconds) {
  static uint32_t lastSeconds = 0, lastMinutes = 0;
  static uint32_t lastHours = 0;
  int16_t x_end, y_end;
  
  // Erase old hour hand if hour changed - draw thick line with parallel offset lines
  if(lastHours != hours) {
    uint16_t old_hour_angle = (lastHours % 12) * 30 + lastMinutes / 2;
    Angle_To_Coords(old_hour_angle, HOUR_HAND_LEN, &x_end, &y_end);
    ST7735_DrawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, x_end, y_end, ST7735_BLACK);
    ST7735_DrawLine(CLOCK_CENTER_X+1, CLOCK_CENTER_Y, x_end+1, y_end, ST7735_BLACK);
    ST7735_DrawLine(CLOCK_CENTER_X-1, CLOCK_CENTER_Y, x_end-1, y_end, ST7735_BLACK);
  }
  
  // Erase old minute hand if minute changed - draw thick line with parallel offset lines
  if(lastMinutes != minutes) {
    uint16_t old_minute_angle = lastMinutes * 6 + lastSeconds / 10;
    Angle_To_Coords(old_minute_angle, MINUTE_HAND_LEN, &x_end, &y_end);
    ST7735_DrawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, x_end, y_end, ST7735_BLACK);
    ST7735_DrawLine(CLOCK_CENTER_X+1, CLOCK_CENTER_Y, x_end+1, y_end, ST7735_BLACK);
    ST7735_DrawLine(CLOCK_CENTER_X-1, CLOCK_CENTER_Y, x_end-1, y_end, ST7735_BLACK);
  }
  
  // Erase old second hand by drawing it in black
  if(lastSeconds != seconds) {
    uint16_t old_second_angle = lastSeconds * 6;
    Angle_To_Coords(old_second_angle, SECOND_HAND_LEN, &x_end, &y_end);
    ST7735_DrawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, x_end, y_end, ST7735_BLACK);
  }
  
  lastHours = hours;
  lastMinutes = minutes;
  lastSeconds = seconds;
  
  // Calculate hour hand angle (30° per hour + 0.5° per minute)
  uint16_t hour_angle = (hours % 12) * 30 + minutes / 2;
  Angle_To_Coords(hour_angle, HOUR_HAND_LEN, &x_end, &y_end);
  ST7735_DrawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, x_end, y_end, ST7735_WHITE);
  
  // Calculate minute hand angle (6° per minute + 0.1° per second)
  uint16_t minute_angle = minutes * 6 + seconds / 10;
  Angle_To_Coords(minute_angle, MINUTE_HAND_LEN, &x_end, &y_end);
  ST7735_DrawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, x_end, y_end, ST7735_CYAN);
  
  // Calculate second hand angle (6° per second)
  uint16_t second_angle = seconds * 6;
  Angle_To_Coords(second_angle, SECOND_HAND_LEN, &x_end, &y_end);
  ST7735_DrawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, x_end, y_end, ST7735_RED);
}

// Main function to draw complete analog clock
void Draw_AnalogClock(uint32_t hours, uint32_t minutes, uint32_t seconds) {
  // Draw clock face (white circle covers old hands) and hands
  Draw_AnalogClockFace();
  Draw_AnalogClockHands(hours, minutes, seconds);
}
