#ifndef __SOUND_H__
#define __SOUND_H__

#include <stdint.h>

void Sound_Init(void);

void Sound_AlarmStart(void);

void Sound_AlarmStop(void);

// Volume control functions
// Read potentiometer raw ADC value (0-4095)
uint32_t Volume_GetPot(void);

// Get volume as a percentage (0-100%)
uint32_t Volume_GetPercent(void);

// Set the speaker volume (0-100%)
// 0 = silent, 100 = full volume
void Volume_SetSpeakerVolume(uint32_t percent);

#endif

