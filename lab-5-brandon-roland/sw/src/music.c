// File **********music.c***********
// Programs to play pre-programmed music and respond to switch inputs
// Spring 2025



#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/SysTickInts.h"
#include "Switch.h"
#include "../inc/TLV5616.h"
#include "music.h"
#include "mailbox.h"
#include "DAC.h"

    // write this
#define QUARTER 2756
#define HALF 5512
#define WHOLE 11025

#define NUM_SONGS 4

// 64 samples of a sine wave (values from 0 to 4095)
const uint16_t SineWave[64] = {
  2048,2248,2447,2642,2831,3013,3185,3346,3495,3630,3750,3853,3939,4007,4056,4085,
  4095,4085,4056,4007,3939,3853,3750,3630,3495,3346,3185,3013,2831,2642,2447,2248,
  2048,1847,1648,1453,1264,1082,910,749,600,465,345,242,156,88,39,10,
  0,10,39,88,156,242,345,465,600,749,910,1082,1264,1453,1648,1847
};

uint32_t CurrentPlaylistIndex = 0;
uint32_t PhaseAccumulator[3] = {0, 0, 0}; // High bits = index, Low bits = fraction
uint32_t CurrentNoteIndex = 0; // Which note in the song we are playing
uint32_t SampleCounter = 0;    // How long we've played the current note
uint32_t EnvelopeVol = 0; 

const Note_t HotCrossBunsScore[] = {
    {{B4, 0, 0}, HALF, 1}, 
    {{A4, 0, 0}, HALF, 1}, 
    {{G4, 0, 0}, WHOLE, 1}, 
    {{B4, 0, 0}, HALF, 1}, 
    {{A4, 0, 0}, HALF, 1}, 
    {{G4, 0, 0}, WHOLE, 1}, 
    {{G4, 0, 0}, QUARTER - QUARTER/10, 1}, 
    {{REST, 0, 0}, QUARTER/10, 0}, // REST uses 0 active voices
    {{G4, 0, 0}, QUARTER - QUARTER/10, 1}, 
    {{REST, 0, 0}, QUARTER/10, 0},
    {{G4, 0, 0}, QUARTER - QUARTER/10, 1}, 
    {{REST, 0, 0}, QUARTER/10, 0},
    {{G4, 0, 0}, QUARTER - QUARTER/10, 1}, 
    {{REST, 0, 0}, QUARTER/10, 0},
    {{A4, 0, 0}, QUARTER - QUARTER/10, 1}, 
    {{REST, 0, 0}, QUARTER/10, 0},
    {{A4, 0, 0}, QUARTER - QUARTER/10, 1}, 
    {{REST, 0, 0}, QUARTER/10, 0},
    {{A4, 0, 0}, QUARTER - QUARTER/10, 1}, 
    {{REST, 0, 0}, QUARTER/10, 0},
    {{A4, 0, 0}, QUARTER - QUARTER/10, 1}, 
    {{REST, 0, 0}, QUARTER/10, 0},
    {{B4, 0, 0}, HALF, 1}, 
    {{A4, 0, 0}, HALF, 1}, 
    {{G4, 0, 0}, WHOLE, 1}
};

const Note_t MaryHadALambScore[] = {
    {{E4, 0, 0}, HALF, 1}, {{D4, 0, 0}, HALF, 1}, {{C4, 0, 0}, HALF, 1}, {{D4, 0, 0}, HALF, 1},
    {{E4, 0, 0}, HALF, 1}, {{E4, 0, 0}, HALF, 1}, {{E4, 0, 0}, WHOLE, 1},
    {{D4, 0, 0}, HALF, 1}, {{D4, 0, 0}, HALF, 1}, {{D4, 0, 0}, WHOLE, 1},
    {{E4, 0, 0}, HALF, 1}, {{G4, 0, 0}, HALF, 1}, {{G4, 0, 0}, WHOLE, 1},
    {{E4, 0, 0}, HALF, 1}, {{D4, 0, 0}, HALF, 1}, {{C4, 0, 0}, HALF, 1}, {{D4, 0, 0}, HALF, 1},
    {{E4, 0, 0}, HALF, 1}, {{E4, 0, 0}, HALF, 1}, {{E4, 0, 0}, HALF, 1}, {{E4, 0, 0}, HALF, 1},
    {{D4, 0, 0}, HALF, 1}, {{D4, 0, 0}, HALF, 1}, {{E4, 0, 0}, HALF, 1}, {{D4, 0, 0}, HALF, 1},
    {{C4, 0, 0}, WHOLE, 1}
};

const Note_t HotCrossBunsChordsScore[] = {
    // B4 Major Chord (B4, G4, E4) - 3 voices
    {{B4, G4, E4}, HALF, 3}, 
    {{A4, G4, E4}, HALF, 3}, 
    {{G4, E4, C4}, WHOLE, 3}, 
    
    // Repeat with 3 voices
    {{B4, G4, E4}, HALF, 3}, 
    {{A4, G4, E4}, HALF, 3}, 
    {{G4, E4, C4}, WHOLE, 3}, 

    // Single notes (1 voice) to show the transition
    {{G4, 0, 0}, QUARTER - QUARTER/10, 1}, 
    {{REST, 0, 0}, QUARTER/10, 0},
    {{G4, 0, 0}, QUARTER - QUARTER/10, 1}, 
    {{REST, 0, 0}, QUARTER/10, 0},
    
    // Final Chord
    {{B4, A4, G4}, WHOLE, 3}
};

const Note_t SilentNightScore[] = {
    {{G4, 0, 0}, QUARTER + HALF, 1}, {{A4, 0, 0}, QUARTER, 1}, {{G4, 0, 0}, HALF, 1}, {{E4, 0, 0}, HALF*3, 1},
    {{G4, 0, 0}, QUARTER + HALF, 1}, {{A4, 0, 0}, QUARTER, 1}, {{G4, 0, 0}, HALF, 1}, {{E4, 0, 0}, HALF*3, 1},
    {{D4, 0, 0}, WHOLE, 1}, {{D4, 0, 0}, HALF, 1}, {{B4, 0, 0}, HALF*3, 1}, {{C4, 0, 0}, HALF*3, 1}, {{C4, 0, 0}, HALF, 1}, {{G4, 0, 0}, HALF*3, 1},
    {{A4, 0, 0}, WHOLE, 1}, {{A4, 0, 0}, HALF, 1}, {{C4, 0, 0}, QUARTER + HALF, 1}, {{B4, 0, 0}, QUARTER, 1}, {{A4, 0, 0}, HALF, 1}, {{G4, 0, 0}, QUARTER + HALF, 1}, {{A4, 0, 0}, QUARTER, 1}, {{G4, 0, 0}, HALF, 1}, {{E4, 0, 0}, HALF*3, 1},
    {{A4, 0, 0}, WHOLE, 1}, {{A4, 0, 0}, HALF, 1}, {{C4, 0, 0}, QUARTER + HALF, 1}, {{B4, 0, 0}, QUARTER, 1}, {{A4, 0, 0}, HALF, 1}, {{G4, 0, 0}, QUARTER + HALF, 1}, {{A4, 0, 0}, QUARTER, 1}, {{G4, 0, 0}, HALF, 1}, {{E4, 0, 0}, HALF*3, 1},
    {{D4, 0, 0}, WHOLE, 1}, {{D4, 0, 0}, HALF, 1}, {{F4, 0, 0}, QUARTER + HALF, 1}, {{D4, 0, 0}, QUARTER, 1}, {{B4, 0, 0}, HALF, 1}, {{C5, 0, 0}, HALF*3, 1}, {{E4, 0, 0}, HALF, 1}, {{0, 0, 0}, HALF, 1}, 
    {{C5, 0, 0}, HALF, 1}, {{G4, 0, 0}, HALF, 1}, {{E4, 0, 0}, HALF, 1}, {{G4, 0, 0}, QUARTER + HALF, 1}, {{F4, 0, 0}, QUARTER, 1}, {{D4, 0, 0}, HALF, 1}, {{C4, 0, 0}, HALF*5, 1}
};

const Song_t HotCrossBuns = {
    HotCrossBunsScore,
    sizeof(HotCrossBunsScore) / sizeof(Note_t),
    "Hot Cross Buns"
};

const Song_t MaryHadALamb = {
    MaryHadALambScore,
    sizeof(MaryHadALambScore) / sizeof(Note_t),
    "Mary Had A Little Lamb"
};

const Song_t HotCrossBunsChords = {
    HotCrossBunsChordsScore,
    sizeof(HotCrossBunsChordsScore) / sizeof(Note_t),
    "Hot Cross Buns - Chord Version"
};

const Song_t SilentNight = {
    SilentNightScore,
    sizeof(SilentNightScore) / sizeof(Note_t),
    "Silent Night"
};

const Song_t* Playlist[] = {
    &HotCrossBuns,
    &MaryHadALamb,
    &HotCrossBunsChords,
    &SilentNight
};

void play_Sound(void) {
    const Song_t *currentSong = Playlist[CurrentPlaylistIndex];
    const Note_t *currentNote = &currentSong->notes[CurrentNoteIndex];

    uint32_t sum = 0;
    uint8_t notes = currentNote->numNotes == 0 ? 1 : currentNote->numNotes;

    // 1. Only process samples and update phase if it's not a REST
    if(currentNote->numNotes > 0) {
        for(int i = 0; i < currentNote->numNotes; i++) {
            uint32_t sineIndex = (PhaseAccumulator[i] >> 26) & 0x3F;
            sum += SineWave[sineIndex];
            PhaseAccumulator[i] += currentNote->delay[i];
        }
        
        // 2. Normalize sum (one time only!)
        sum = sum / notes; 

        // 3. Calculate Envelope Volume (ADSR-lite)
        if (SampleCounter < 500) { 
            EnvelopeVol = (SampleCounter * 256) / 500; // Attack
        } else if (SampleCounter > (currentNote->duration - 500)) {
            EnvelopeVol = ((currentNote->duration - SampleCounter) * 256) / 500; // Release
        } else {
            EnvelopeVol = 256; // Sustain
        }
    } else {
        sum = 0; // Ensure silence during REST
        EnvelopeVol = 0;
    }

    // 4. Final Output: Apply envelope to the normalized sum
    dac_output((sum * EnvelopeVol) / 256); 

    // 5. Timing and Transitions
    SampleCounter++;
    if(SampleCounter >= currentNote->duration) {
        SampleCounter = 0;
        for(int i = 0; i < 3; i++) PhaseAccumulator[i] = 0; // Reset
        CurrentNoteIndex++;   
        
        if(CurrentNoteIndex >= currentSong->numNotes) {
            CurrentNoteIndex = 0; // Loop song
        }
    }
}

//-------------- Music_Init ----------------
// activate periodic interrupts and DAC
// Inputs: none
// Outputs: none
// called once

void Music_Init(void){
    dac_init();
    SysTick_Init(7256, &play_Sound); 
}

void Music_Start(uint32_t index) {
    CurrentPlaylistIndex = index;
    CurrentNoteIndex = 0;
    SampleCounter = 0;
    for(int i = 0; i < 3; i++) PhaseAccumulator[i] = 0;
    
    NVIC_ST_CTRL_R |= 0x01; // Enable the timer bit
}

void Music_Stop(void) {
    NVIC_ST_CTRL_R &= ~0x01; // Disable the timer bit
    dac_output(0);           // Silence
}

void Music_Resume(){
    NVIC_ST_CTRL_R |= 0x01;
}

void Music_Next(void) {
    Music_Stop(); // 1. Silence the current song and stop the timer
    
    // 2. Increment index and wrap around if at the end
    CurrentPlaylistIndex = (CurrentPlaylistIndex + 1) % NUM_SONGS; 
    
    // 3. Start the new song
    Music_Start(CurrentPlaylistIndex);
}