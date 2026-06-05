#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

#include "Config.h"

enum OscType : uint8_t {
  OSC_SINE = 0,
  OSC_TRIANGLE,
  OSC_SQUARE,
  OSC_SAW
};

enum ScreenMode : uint8_t {
  MODE_MAIN = 0,
  MODE_TRACK,
  MODE_MASTER_VOL,
  MODE_TRACK_VOL,
  MODE_BPM,
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
  int8_t note;       // -1 = empty, 0~7 = C D E F G A B C
  uint8_t velocity;  // 0~127
};

struct Track {
  Step steps[STEP_COUNT];
  bool mute;
  uint8_t volume;  // 0~127
  OscType osc;
};

struct Voice {
  bool active;
  uint8_t track;
  uint32_t phase;
  uint32_t inc;
  OscType osc;
  int32_t ageSamples;
  int32_t gateSamples;
};

#endif
