#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <Arduino.h>
#include <ESP_I2S.h>

#include "Types.h"

class AudioEngine {
public:
  void begin();
  void task();
  void triggerVoice(uint8_t track, int8_t note, OscType osc, int8_t pitchOffset, uint32_t samplesPerStep);
  int16_t renderSample();

private:
  I2SClass i2s;

  int16_t renderOscillator(Voice &v);
  int16_t renderDrum(Voice &v);
  int32_t envelopeQ15(Voice &v);
  int32_t drumEnvelopeQ15(Voice &v);
};

extern AudioEngine audioEngine;

void audioTask(void *param);

#endif
