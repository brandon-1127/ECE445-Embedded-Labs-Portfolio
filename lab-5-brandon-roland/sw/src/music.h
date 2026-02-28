// File **********music.h***********
// Lab 5
// Programs to play pre-programmed music and respond to switch
// inputs.
// EE445L Spring 2025

#ifndef __MUSIC_H__
#define __MUSIC_H__

#include <stdint.h>

// Note structure
typedef struct {
    uint32_t delay[3];    // Step size / Phase increment
    uint32_t duration; // Time in samples
    uint8_t numNotes;
} Note_t;

// Song structure
typedef struct {
    const Note_t *notes; 
    uint32_t numNotes;
    const char *name;
} Song_t;

// Global Playlist Index - allows GPIO handler to see which song is selected
extern uint32_t CurrentPlaylistIndex;

// Function Prototypes
void Music_Init(void);
void Music_Start(uint32_t index);
void Music_Stop(void);
void Music_Resume(void);
void Music_Next(void);

// Pitch definitions (Step Sizes for 11.025kHz)
#define C4  101894845
#define CS4 107950293
#define D4  114360705
#define DS4 121172151
#define E4  128375983
#define F4  136009481
#define FS4 144094503
#define G4  152664576
#define GS4 161738749
#define A4  171363205
#define AS4 181551139
#define B4  192347159
#define C5  203789691
#define REST 0

#endif // __MUSIC_H__

