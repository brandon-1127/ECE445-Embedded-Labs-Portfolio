// NTC_Thermistor.h
// Runs on LM4F120/TM4C123
// Provides functions to read a 10k NTC thermistor connected to an ADC pin
// and convert the reading to temperature in Celsius
// Uses a voltage divider: Vcc--[R_fixed]--[ADC_pin]--[NTC_thermistor]--GND
// 
// Basic usage:
//   NTC_Init();                      // Initialize ADC
//   int32_t temp = NTC_GetTemp();    // Read temperature in Celsius (x10)
//
// The NTC thermistor uses the Steinhart-Hart equation for accuracy

#include <stdint.h>

// Initialize ADC for thermistor reading
// Configures ADC0 on a specific channel for the thermistor
void NTC_Init(void);

// Read raw ADC value from thermistor
// Returns: 12-bit ADC value (0-4095)
uint32_t NTC_ReadRaw(void);

// Convert ADC reading to resistance in Ohms
// Assumes 10k fixed resistor in voltage divider
// Returns: Resistance in Ohms
uint32_t NTC_ADCtoResistance(uint32_t adc_value);

// Get temperature reading from NTC thermistor
// Uses Steinhart-Hart equation for conversion
// Returns: Temperature in Fahrenheit * 10 (e.g., 775 = 77.5°F)
int32_t NTC_GetTemp(void);

// Get temperature in Celsius with one decimal place as a string
// buf: pointer to buffer (at least 6 characters: "-XX.X\0")
void NTC_GetTempString(char* buf);
