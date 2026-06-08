#include "Sequencer.h"

#include "AudioEngine.h"
#include "State.h"

Sequencer sequencer;

void Sequencer::advanceFromAudio(uint32_t samplesPerStep) {
  uint8_t newStep;
  uint8_t oldStep;

  int8_t notesToTrigger[TRACK_COUNT];
  OscType oscToTrigger[TRACK_COUNT];
  bool muteToUse[TRACK_COUNT];

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    notesToTrigger[t] = -1;
    oscToTrigger[t] = OSC_SINE;
    muteToUse[t] = true;
  }

  portENTER_CRITICAL(&stateMux);

  uint8_t stepsInLoop = activeStepCountNoLock();

  oldStep = currentStep;
  currentStep = (currentStep + 1) % stepsInLoop;
  newStep = currentStep;
  previousStep = oldStep;
  bool startedRecording = false;

  setDirtyColNoLock(oldStep);
  setDirtyColNoLock(newStep);
  setDirtyStatusNoLock();

  if (newStep == 0) {
    setDirtyFullNoLock();
  }

  // Record armed: wait until the loop reaches step 0.
  if (screenMode == MODE_REC_ARMED && newStep == 0) {
    screenMode = MODE_RECORDING;
    recordCount = 0;
    recordLatchedNote = -1;
    clearTrackNoLock(recordTrack);
    setDirtyFullNoLock();
    startedRecording = true;
  }

  // Recording: write any note held during the step that just ended.
  if (screenMode == MODE_RECORDING && !startedRecording) {
    int8_t n = recordLatchedNote;

    tracks[recordTrack].steps[oldStep].note = n;
    tracks[recordTrack].steps[oldStep].velocity = (n >= 0) ? 100 : 0;

    recordLatchedNote = -1;

    setDirtyCellNoLock(recordTrack, oldStep);
    setDirtyStatusNoLock();

    recordCount++;

    if (recordCount >= stepsInLoop) {
      screenMode = MODE_TRACK;
      trackMenuIndex = 1;
      recordCount = 0;
      setDirtyFullNoLock();
    }
  }

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    notesToTrigger[t] = tracks[t].steps[newStep].note;
    oscToTrigger[t] = tracks[t].osc;
    muteToUse[t] = tracks[t].mute;
  }

  portEXIT_CRITICAL(&stateMux);

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    if (!muteToUse[t] && notesToTrigger[t] >= 0) {
      audioEngine.triggerVoice(t, notesToTrigger[t], oscToTrigger[t], samplesPerStep);
    }
  }
}
