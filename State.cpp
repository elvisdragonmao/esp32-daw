#include "State.h"

#include <math.h>

portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

Track tracks[TRACK_COUNT];
Voice voices[TRACK_COUNT];

volatile bool playing = false;
volatile bool resetPlayback = false;

volatile uint8_t currentStep = 0;
volatile uint8_t previousStep = 15;

volatile uint16_t bpm = 120;
volatile uint8_t masterVolume = 100;
volatile uint8_t barCount = DEFAULT_BAR_COUNT;

volatile ScreenMode screenMode = MODE_MAIN;

volatile uint8_t mainIndex = 0;
volatile uint8_t selectedTrack = 0;
volatile uint8_t trackMenuIndex = 0;
volatile uint8_t oscMenuIndex = 0;

volatile uint8_t recordTrack = 0;
volatile uint8_t recordCount = 0;
volatile int8_t currentRecNote = -1;
volatile int8_t recordLatchedNote = -1;

volatile bool dirtyFull = true;
volatile bool dirtyMenu = true;
volatile bool dirtyStatus = true;
volatile bool dirtyCol[STEP_COUNT];
volatile bool dirtyCell[TRACK_COUNT][STEP_COUNT];

int16_t sineTable[256];

const float NOTE_FREQS[8] = {
  261.63f, 293.66f, 329.63f, 349.23f,
  392.00f, 440.00f, 493.88f, 523.25f
};

const char *const NOTE_NAMES[8] = {
  "C", "D", "E", "F", "G", "A", "B", "C+"
};

const char *const DRUM_NAMES[DRUM_SOUND_COUNT] = {
  "Kick", "Snare", "Hat", "Tom"
};

uint16_t COL_BG;
uint16_t COL_PANEL;
uint16_t COL_PANEL_DARK;
uint16_t COL_TEXT;
uint16_t COL_MUTED_TEXT;
uint16_t COL_SELECT;
uint16_t COL_GRID_EMPTY;
uint16_t COL_GRID_BORDER;
uint16_t COL_NOTE;
uint16_t COL_MUTED_NOTE;
uint16_t COL_PLAYHEAD;
uint16_t COL_RECORD;

const char *oscName(OscType osc) {
  switch (osc) {
    case OSC_SINE: return "Sin";
    case OSC_TRIANGLE: return "Tri";
    case OSC_SQUARE: return "Sqr";
    case OSC_SAW: return "Saw";
    case OSC_DRUM: return "Drum";
    default: return "?";
  }
}

const char *recordNoteName(OscType osc, int8_t note) {
  if (note < 0) return "--";

  if (osc == OSC_DRUM) {
    return DRUM_NAMES[note & 0x03];
  }

  if (note < 8) {
    return NOTE_NAMES[note];
  }

  return "?";
}

uint32_t freqToInc(float freq) {
  return (uint32_t)((double)freq * 4294967296.0 / (double)SAMPLE_RATE);
}

uint8_t activeStepCountNoLock() {
  uint8_t bars = barCount;

  if (bars < MIN_BAR_COUNT) bars = MIN_BAR_COUNT;
  if (bars > MAX_BAR_COUNT) bars = MAX_BAR_COUNT;

  return bars * STEPS_PER_BAR;
}

uint8_t activeStepCount() {
  portENTER_CRITICAL(&stateMux);
  uint8_t steps = activeStepCountNoLock();
  portEXIT_CRITICAL(&stateMux);

  return steps;
}

void setDirtyFullNoLock() {
  dirtyFull = true;
}

void setDirtyStatusNoLock() {
  dirtyStatus = true;
}

void setDirtyMenuNoLock() {
  dirtyMenu = true;
}

void setDirtyCellNoLock(uint8_t t, uint8_t s) {
  if (t < TRACK_COUNT && s < STEP_COUNT) {
    dirtyCell[t][s] = true;
  }
}

void setDirtyColNoLock(uint8_t s) {
  if (s < STEP_COUNT) {
    dirtyCol[s] = true;
  }
}

void clearTrackNoLock(uint8_t t) {
  for (uint8_t s = 0; s < STEP_COUNT; s++) {
    tracks[t].steps[s].note = -1;
    tracks[t].steps[s].velocity = 0;
    dirtyCell[t][s] = true;
  }
}

int clampInt(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void initSineTable() {
  for (int i = 0; i < 256; i++) {
    float phase = (float)i * 2.0f * PI / 256.0f;
    sineTable[i] = (int16_t)(sin(phase) * 32767.0f);
  }
}

void initTracks() {
  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    tracks[t].mute = false;
    tracks[t].volume = 100;
    tracks[t].pitchOffset = 0;
    tracks[t].osc = (OscType)t;

    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      tracks[t].steps[s].note = -1;
      tracks[t].steps[s].velocity = 0;
    }

    voices[t].active = false;
    voices[t].track = t;
    voices[t].phase = 0;
    voices[t].inc = 0;
    voices[t].noiseState = 0x13579BDFUL + ((uint32_t)t * 0x2468ACEUL);
    voices[t].osc = (OscType)t;
    voices[t].drum = 0;
    voices[t].ageSamples = 0;
    voices[t].gateSamples = 0;
  }

  // Demo pattern for quick audio feedback after boot.
  tracks[0].steps[0].note = 0;
  tracks[0].steps[4].note = 2;
  tracks[0].steps[8].note = 4;
  tracks[0].steps[12].note = 7;

  tracks[1].steps[2].note = 4;
  tracks[1].steps[6].note = 5;
  tracks[1].steps[10].note = 4;
  tracks[1].steps[14].note = 2;

  tracks[2].steps[0].note = 0;
  tracks[2].steps[8].note = 0;

  tracks[3].steps[3].note = 7;
  tracks[3].steps[7].note = 6;
  tracks[3].steps[11].note = 5;
  tracks[3].steps[15].note = 4;
}
