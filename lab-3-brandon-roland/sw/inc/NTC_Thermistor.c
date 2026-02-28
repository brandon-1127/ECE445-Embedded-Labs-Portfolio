// NTC_Thermistor.c
// Runs on LM4F120/TM4C123
// Reads a 10k NTC thermistor and converts to temperature
// Uses ADC0 SS3 software trigger for reading
// Steinhart-Hart equation for accurate temperature conversion

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ADCSWTrigger.h"

// NTC Thermistor parameters (typical 10k NTC at 25°C)
// B value: sensitivity parameter (typically 3000-4000 for 10k NTC)
#define NTC_B_VALUE 3950      // B coefficient in Kelvin
#define NTC_R0 10000          // Reference resistance at 25°C (10k)
#define NTC_T0 298.15         // Reference temperature in Kelvin (25°C)
#define NTC_FIXED_R 10000     // Fixed resistor in voltage divider (10k)
#define NTC_VCC 3300          // Supply voltage in mV

// Initialize ADC0 for thermistor reading
void NTC_Init(void) {
  // Initialize ADC0 SS3 with software trigger on channel 9 (PE4)
  ADC0_InitSWTriggerSeq3_Ch9();
}

// Read raw ADC value
uint32_t NTC_ReadRaw(void) {
  return ADC0_InSeq3();
}

// Convert ADC reading to resistance
// ADC voltage = (adc_value / 4096) * 3.3V
// Using voltage divider: R_ntc = R_fixed * (V_cc - V_adc) / V_adc
uint32_t NTC_ADCtoResistance(uint32_t adc_value) {
  if (adc_value >= 4095) adc_value = 4094;  // Prevent division issues
  if (adc_value <= 0) adc_value = 1;
  
  // R_ntc = R_fixed * (4096 - adc_value) / adc_value
  uint32_t resistance = (uint32_t)(NTC_FIXED_R * (4095 - adc_value) / adc_value);
  
  return resistance;
}

// Convert resistance to temperature using Steinhart-Hart equation
// 1/T = 1/T0 + (1/B) * ln(R/R0)
// where: T = absolute temperature (Kelvin)
//        T0 = reference temperature (298.15K for 25°C)
//        B = B value of thermistor
//        R = measured resistance
//        R0 = reference resistance (10k at 25°C)
static int32_t NTC_ResistanceToTemp(uint32_t resistance) {
  // Simplified calculation using approximation
  // For better accuracy, use full Steinhart-Hart equation
  
  double r = (double)resistance;
  double r0 = (double)NTC_R0;
  double b = (double)NTC_B_VALUE;
  double t0 = NTC_T0;  // 298.15K
  
  // Steinhart-Hart: 1/T = 1/T0 + (1/B)*ln(R/R0)
  double ln_ratio = log(r / r0);
  double inv_t = (1.0 / t0) + (1.0 / b) * ln_ratio;
  double temp_k = 1.0 / inv_t;
  
  // Convert Kelvin to Celsius, then to Fahrenheit
  double temp_c = temp_k - 273.15;
  double temp_f = temp_c * 9.0 / 5.0 + 32.0;
  
  // Return temperature * 10 for one decimal place precision
  return (int32_t)(temp_f * 10.0);
}

// Get temperature reading
int32_t NTC_GetTemp(void) {
  uint32_t adc_raw = NTC_ReadRaw();
  uint32_t resistance = NTC_ADCtoResistance(adc_raw);
  int32_t temperature = NTC_ResistanceToTemp(resistance);
  
  return temperature;
}

// Get temperature as a formatted string
void NTC_GetTempString(char* buf) {
  int32_t temp = NTC_GetTemp();
  
  // temp is in units of 0.1°C
  int32_t whole = temp / 10;
  int32_t decimal = temp % 10;
  
  // Handle negative temperatures
  if (decimal < 0) decimal = -decimal;
  
  sprintf(buf, "%d.%d", whole, decimal);
}
