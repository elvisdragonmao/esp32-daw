#include "AudioEngine.h"

#include "Sequencer.h"
#include "State.h"

AudioEngine audioEngine;

static const uint32_t DRUM_KICK_MIN_INC = 8765239UL;
static const uint32_t DRUM_TOM_MIN_INC = 15582647UL;

static uint32_t drumFreqToInc(uint16_t freq) {
  return (uint32_t)(((uint64_t)freq * 4294967296ULL) / SAMPLE_RATE);
}

void AudioEngine::begin() {
  pinMode(PIN_I2S_SD, OUTPUT);
  digitalWrite(PIN_I2S_SD, HIGH);

  // BCLK, WS/LRC, DOUT, DIN, MCLK.
  // DIN and MCLK are unused, so they stay at -1.
  i2s.setPins(
    PIN_I2S_BCLK,
    PIN_I2S_LRC,
    PIN_I2S_DOUT,
    -1,
    -1);

  bool ok = i2s.begin(
    I2S_MODE_STD,
    SAMPLE_RATE,
    I2S_DATA_BIT_WIDTH_16BIT,
    I2S_SLOT_MODE_STEREO);

  if (!ok) {
    Serial.println("I2S begin failed");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("I2S OK");
}

int16_t AudioEngine::renderOscillator(Voice &v) {
  uint32_t ph = v.phase;

  switch (v.osc) {
    case OSC_SINE:
      {
        return sineTable[ph >> 24];
      }

    case OSC_TRIANGLE:
      {
        uint16_t p = ph >> 16;
        int32_t tri;

        if (p < 32768) {
          tri = (int32_t)p * 2 - 32768;
        } else {
          tri = 98304 - (int32_t)p * 2;
        }

        return (int16_t)tri;
      }

    case OSC_SQUARE:
      {
        return (ph & 0x80000000UL) ? -32767 : 32767;
      }

    case OSC_SAW:
      {
        return (int16_t)((int32_t)(ph >> 16) - 32768);
      }

    case OSC_DRUM:
      {
        return renderDrum(v);
      }

    default:
      return 0;
  }
}

int16_t AudioEngine::renderDrum(Voice &v) {
  v.noiseState ^= v.noiseState << 13;
  v.noiseState ^= v.noiseState >> 17;
  v.noiseState ^= v.noiseState << 5;

  int16_t noise = (int16_t)(v.noiseState >> 16);
  int16_t tone = sineTable[v.phase >> 24];

  switch (v.drum & 0x03) {
    case 0:
      if (v.inc > DRUM_KICK_MIN_INC) {
        v.inc -= v.inc >> 7;
      }
      return tone;

    case 1:
      return (int16_t)(((int32_t)noise * 3 + (tone >> 1)) / 4);

    case 2:
      return (v.noiseState & 0x8000UL) ? 22000 : -22000;

    case 3:
      if (v.inc > DRUM_TOM_MIN_INC) {
        v.inc -= v.inc >> 8;
      }
      return (int16_t)((tone >> 1) + (noise >> 3));
  }

  return 0;
}

int32_t AudioEngine::drumEnvelopeQ15(Voice &v) {
  static const int32_t decaySamples[DRUM_SOUND_COUNT] = {
    SAMPLE_RATE * 150 / 1000,
    SAMPLE_RATE * 100 / 1000,
    SAMPLE_RATE * 45 / 1000,
    SAMPLE_RATE * 180 / 1000
  };

  int32_t decay = decaySamples[v.drum & 0x03];

  if (v.ageSamples >= decay) {
    v.active = false;
    return 0;
  }

  return ((decay - v.ageSamples) * 32767) / decay;
}

int32_t AudioEngine::envelopeQ15(Voice &v) {
  if (v.osc == OSC_DRUM) {
    return drumEnvelopeQ15(v);
  }

  const int32_t attackSamples = SAMPLE_RATE * 5 / 1000;
  const int32_t releaseSamples = SAMPLE_RATE * 60 / 1000;

  if (v.ageSamples < attackSamples) {
    return (v.ageSamples * 32767) / attackSamples;
  }

  if (v.ageSamples < v.gateSamples) {
    return 32767;
  }

  int32_t releaseAge = v.ageSamples - v.gateSamples;

  if (releaseAge < releaseSamples) {
    return ((releaseSamples - releaseAge) * 32767) / releaseSamples;
  }

  v.active = false;
  return 0;
}

void AudioEngine::triggerVoice(uint8_t track, int8_t note, OscType osc, uint32_t samplesPerStep) {
  if (track >= TRACK_COUNT || note < 0 || note > 7) return;

  voices[track].active = true;
  voices[track].track = track;
  voices[track].phase = 0;
  voices[track].noiseState ^= 0xA5A5A5A5UL ^ ((uint32_t)track << 24) ^ ((uint32_t)note << 16);
  voices[track].osc = osc;
  voices[track].drum = note & 0x03;
  voices[track].ageSamples = 0;

  if (osc == OSC_DRUM) {
    static const uint16_t drumFreqs[DRUM_SOUND_COUNT] = {110, 180, 8000, 150};
    voices[track].inc = drumFreqToInc(drumFreqs[voices[track].drum]);
    voices[track].gateSamples = samplesPerStep;
    return;
  }

  voices[track].inc = freqToInc(NOTE_FREQS[note]);

  int32_t gate = (int32_t)(samplesPerStep * 75 / 100);
  if (gate < 1) gate = 1;
  voices[track].gateSamples = gate;
}

int16_t AudioEngine::renderSample() {
  int32_t mix = 0;

  uint8_t localMaster = masterVolume;

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    Voice &v = voices[t];

    if (!v.active) continue;
    if (tracks[t].mute) continue;

    int16_t osc = renderOscillator(v);
    int32_t env = envelopeQ15(v);

    if (env <= 0) continue;

    int32_t sample = ((int32_t)osc * env) >> 15;
    sample = (sample * tracks[t].volume) / 127;

    mix += sample;

    v.phase += v.inc;
    v.ageSamples++;
  }

  mix = (mix * localMaster) / 127;

  // 4-track headroom to avoid clipping.
  mix /= 4;

  if (mix > 32767) mix = 32767;
  if (mix < -32768) mix = -32768;

  return (int16_t)mix;
}

void AudioEngine::task() {
  int16_t audioBuffer[AUDIO_FRAMES * 2];
  uint32_t samplesToNextStep = 1;

  while (true) {
    bool localPlaying;
    bool localReset;
    uint16_t localBpm;

    portENTER_CRITICAL(&stateMux);
    localPlaying = playing;
    localReset = resetPlayback;
    localBpm = bpm;

    if (resetPlayback) {
      resetPlayback = false;
      currentStep = 15;
      previousStep = 15;
      setDirtyFullNoLock();
    }
    portEXIT_CRITICAL(&stateMux);

    if (localReset) {
      samplesToNextStep = 0;
    }

    uint32_t samplesPerStep = ((uint64_t)SAMPLE_RATE * 60) / localBpm;
    if (samplesPerStep < 100) samplesPerStep = 100;

    for (uint16_t i = 0; i < AUDIO_FRAMES; i++) {
      if (localPlaying) {
        if (samplesToNextStep == 0) {
          sequencer.advanceFromAudio(samplesPerStep);
          samplesToNextStep = samplesPerStep;
        }

        samplesToNextStep--;
      }

      int16_t s = renderSample();

      // Stereo frame with the same sample on both channels.
      // This keeps MAX98357A output reliable regardless of L/R wiring.
      audioBuffer[i * 2] = s;
      audioBuffer[i * 2 + 1] = s;
    }

    size_t bytesWritten = i2s.write((uint8_t *)audioBuffer, sizeof(audioBuffer));

    if (bytesWritten == 0) {
      taskYIELD();
    }

    taskYIELD();
  }
}

void audioTask(void *param) {
  (void)param;
  audioEngine.task();
}
