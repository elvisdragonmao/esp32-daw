#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <Arduino.h>

class Sequencer {
public:
  void advanceFromAudio(uint32_t samplesPerStep);
};

extern Sequencer sequencer;

#endif
