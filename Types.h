#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

#include "Config.h"

enum OscType : uint8_t {
  OSC_SINE = 0,
  OSC_TRIANGLE,
  OSC_SQUARE,
  OSC_SAW,
  OSC_DRUM
};

static const uint8_t OSC_TYPE_COUNT = 5;
static const uint8_t DRUM_SOUND_COUNT = 4;

enum ScreenMode : uint8_t {
  MODE_MAIN = 0,
  MODE_TRACK,
  MODE_MASTER_VOL,
  MODE_TRACK_VOL,
  MODE_TRACK_PITCH,
  MODE_BPM,
  MODE_BARS,
  MODE_OSC,
  MODE_REC_ARMED,
  MODE_RECORDING
};

enum Dir4 : uint8_t {
  DIR_CENTER = 0,
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT
};

struct Step {
  int8_t note;       // -1 = empty, 0~7 = note slot or drum slot
  uint8_t velocity;  // 0~127
};

struct Track {
  Step steps[STEP_COUNT];
  bool mute;
  uint8_t volume;  // 0~127
  int8_t pitchOffset;  // -16, -8, 0, +8, +16 scale-degree transpose
  OscType osc;
};

struct Voice {
  bool active;
  uint8_t track;
  uint32_t phase;
  uint32_t inc;
  uint32_t noiseState;
  OscType osc;
  uint8_t drum;
  int32_t ageSamples;
  int32_t gateSamples;
};

#endif
