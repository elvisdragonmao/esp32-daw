#ifndef STATE_H
#define STATE_H

#include <Arduino.h>

#include "Types.h"

extern portMUX_TYPE stateMux;

extern Track tracks[TRACK_COUNT];
extern Voice voices[TRACK_COUNT];

extern volatile bool playing;
extern volatile bool resetPlayback;

extern volatile uint8_t currentStep;
extern volatile uint8_t previousStep;

extern volatile uint16_t bpm;
extern volatile uint8_t masterVolume;

extern volatile ScreenMode screenMode;

extern volatile uint8_t mainIndex;
extern volatile uint8_t selectedTrack;
extern volatile uint8_t trackMenuIndex;
extern volatile uint8_t oscMenuIndex;

extern volatile uint8_t recordTrack;
extern volatile uint8_t recordCount;
extern volatile int8_t currentRecNote;

extern volatile bool dirtyFull;
extern volatile bool dirtyMenu;
extern volatile bool dirtyStatus;
extern volatile bool dirtyCol[STEP_COUNT];
extern volatile bool dirtyCell[TRACK_COUNT][STEP_COUNT];

extern int16_t sineTable[256];

extern const float NOTE_FREQS[8];
extern const char *const NOTE_NAMES[8];
extern const char *const DRUM_NAMES[DRUM_SOUND_COUNT];

extern uint16_t COL_BG;
extern uint16_t COL_PANEL;
extern uint16_t COL_PANEL_DARK;
extern uint16_t COL_TEXT;
extern uint16_t COL_MUTED_TEXT;
extern uint16_t COL_SELECT;
extern uint16_t COL_GRID_EMPTY;
extern uint16_t COL_GRID_BORDER;
extern uint16_t COL_NOTE;
extern uint16_t COL_MUTED_NOTE;
extern uint16_t COL_PLAYHEAD;
extern uint16_t COL_RECORD;

const char *oscName(OscType osc);
const char *recordNoteName(OscType osc, int8_t note);
uint32_t freqToInc(float freq);

void setDirtyFullNoLock();
void setDirtyStatusNoLock();
void setDirtyMenuNoLock();
void setDirtyCellNoLock(uint8_t t, uint8_t s);
void setDirtyColNoLock(uint8_t s);
void clearTrackNoLock(uint8_t t);

int clampInt(int v, int lo, int hi);

void initSineTable();
void initTracks();

#endif
