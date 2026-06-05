#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <Arduino.h>
#include <ESP_I2S.h>

#include "Types.h"

class AudioEngine {
public:
  void begin();
  void task();
  void triggerVoice(uint8_t track, int8_t note, OscType osc, uint32_t samplesPerStep);
  int16_t renderSample();

private:
  I2SClass i2s;

  int16_t renderOscillator(Voice &v);
  int32_t envelopeQ15(Voice &v);
};

extern AudioEngine audioEngine;

void audioTask(void *param);

#endif
