#include "InputController.h"

#include "Config.h"
#include "Joystick.h"
#include "State.h"

InputController inputController;

void InputController::handleDirEvent(Dir4 dir) {
  portENTER_CRITICAL(&stateMux);

  ScreenMode mode = screenMode;

  if (mode == MODE_MAIN) {
    if (dir == DIR_UP) {
      mainIndex = (mainIndex == 0) ? 7 : mainIndex - 1;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_DOWN) {
      mainIndex = (mainIndex + 1) % 8;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_RIGHT) {
      if (mainIndex < 4) {
        selectedTrack = mainIndex;
        screenMode = MODE_TRACK;
        trackMenuIndex = 0;
        setDirtyFullNoLock();
      }

      else if (mainIndex == 4) {
        if (!playing) {
          playing = true;
          resetPlayback = true;
        } else {
          playing = false;
        }

        setDirtyFullNoLock();
      }

      else if (mainIndex == 5) {
        screenMode = MODE_MASTER_VOL;
        setDirtyFullNoLock();
      }

      else if (mainIndex == 6) {
        screenMode = MODE_BPM;
        setDirtyFullNoLock();
      }

      else if (mainIndex == 7) {
        screenMode = MODE_BARS;
        setDirtyFullNoLock();
      }
    }
  }

  else if (mode == MODE_TRACK) {
    if (dir == DIR_UP) {
      trackMenuIndex = (trackMenuIndex == 0) ? 3 : trackMenuIndex - 1;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_DOWN) {
      trackMenuIndex = (trackMenuIndex + 1) % 4;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_LEFT) {
      screenMode = MODE_MAIN;
      setDirtyFullNoLock();
    }

    else if (dir == DIR_RIGHT) {
      if (trackMenuIndex == 0) {
        tracks[selectedTrack].mute = !tracks[selectedTrack].mute;
        setDirtyFullNoLock();
      }

      else if (trackMenuIndex == 1) {
        recordTrack = selectedTrack;
        recordCount = 0;
        currentRecNote = -1;
        screenMode = MODE_REC_ARMED;

        if (!playing) {
          playing = true;
          resetPlayback = true;
        }

        setDirtyFullNoLock();
      }

      else if (trackMenuIndex == 2) {
        screenMode = MODE_TRACK_VOL;
        setDirtyFullNoLock();
      }

      else if (trackMenuIndex == 3) {
        screenMode = MODE_OSC;
        oscMenuIndex = tracks[selectedTrack].osc;
        setDirtyFullNoLock();
      }
    }
  }

  else if (mode == MODE_MASTER_VOL) {
    if (dir == DIR_UP) {
      masterVolume = clampInt(masterVolume + 5, 0, 127);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_DOWN) {
      masterVolume = clampInt(masterVolume - 5, 0, 127);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_LEFT || dir == DIR_RIGHT) {
      screenMode = MODE_MAIN;
      setDirtyFullNoLock();
    }
  }

  else if (mode == MODE_TRACK_VOL) {
    if (dir == DIR_UP) {
      tracks[selectedTrack].volume = clampInt(tracks[selectedTrack].volume + 5, 0, 127);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_DOWN) {
      tracks[selectedTrack].volume = clampInt(tracks[selectedTrack].volume - 5, 0, 127);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_LEFT || dir == DIR_RIGHT) {
      screenMode = MODE_TRACK;
      setDirtyFullNoLock();
    }
  }

  else if (mode == MODE_BPM) {
    if (dir == DIR_UP) {
      bpm = clampInt(bpm + 5, 40, 240);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_DOWN) {
      bpm = clampInt(bpm - 5, 40, 240);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_LEFT || dir == DIR_RIGHT) {
      screenMode = MODE_MAIN;
      setDirtyFullNoLock();
    }
  }

  else if (mode == MODE_BARS) {
    if (dir == DIR_UP) {
      barCount = clampInt(barCount + 1, MIN_BAR_COUNT, MAX_BAR_COUNT);
      if (currentStep >= activeStepCountNoLock()) {
        resetPlayback = true;
      }
      setDirtyFullNoLock();
    }

    else if (dir == DIR_DOWN) {
      barCount = clampInt(barCount - 1, MIN_BAR_COUNT, MAX_BAR_COUNT);
      if (currentStep >= activeStepCountNoLock()) {
        resetPlayback = true;
      }
      setDirtyFullNoLock();
    }

    else if (dir == DIR_LEFT || dir == DIR_RIGHT) {
      screenMode = MODE_MAIN;
      setDirtyFullNoLock();
    }
  }

  else if (mode == MODE_OSC) {
    if (dir == DIR_UP) {
      oscMenuIndex = (oscMenuIndex == 0) ? OSC_TYPE_COUNT - 1 : oscMenuIndex - 1;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_DOWN) {
      oscMenuIndex = (oscMenuIndex + 1) % OSC_TYPE_COUNT;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_LEFT) {
      screenMode = MODE_TRACK;
      setDirtyFullNoLock();
    }

    else if (dir == DIR_RIGHT) {
      tracks[selectedTrack].osc = (OscType)oscMenuIndex;
      screenMode = MODE_TRACK;
      setDirtyFullNoLock();
    }
  }

  // REC_ARMED / RECORDING use joystick direction for notes, not menu events.

  portEXIT_CRITICAL(&stateMux);
}

void InputController::task() {
  Dir4 heldDir = DIR_CENTER;
  Dir4 pendingDir = DIR_CENTER;
  uint32_t holdStartMs = 0;
  uint32_t lastRepeatMs = 0;
  uint32_t pendingStartMs = 0;

  int8_t stableRecNote = -1;
  int8_t pendingRecNote = -1;
  uint32_t recPendingStartMs = 0;

  while (true) {
    ScreenMode mode = screenMode;

    if (mode == MODE_REC_ARMED || mode == MODE_RECORDING) {
      int8_t rawNote = joystick.readRecordNote8();
      uint32_t now = millis();

      if (rawNote != pendingRecNote) {
        pendingRecNote = rawNote;
        recPendingStartMs = now;
      }

      if (now - recPendingStartMs >= JOY_REC_STABLE_MS && rawNote != stableRecNote) {
        stableRecNote = rawNote;

        portENTER_CRITICAL(&stateMux);
        currentRecNote = stableRecNote;
        setDirtyStatusNoLock();
        portEXIT_CRITICAL(&stateMux);
      }

      vTaskDelay(20 / portTICK_PERIOD_MS);
      continue;
    }

    stableRecNote = -1;
    pendingRecNote = -1;
    recPendingStartMs = 0;

    Dir4 dir = joystick.readDir4();
    uint32_t now = millis();

    if (dir == DIR_CENTER) {
      heldDir = DIR_CENTER;
      pendingDir = DIR_CENTER;
      holdStartMs = 0;
      lastRepeatMs = 0;
      pendingStartMs = 0;
      vTaskDelay(15 / portTICK_PERIOD_MS);
      continue;
    }

    if (dir != heldDir) {
      if (dir != pendingDir) {
        pendingDir = dir;
        pendingStartMs = now;
      }

      if (now - pendingStartMs >= JOY_DIR_STABLE_MS) {
        heldDir = dir;
        pendingDir = DIR_CENTER;
        holdStartMs = now;
        lastRepeatMs = now;
        handleDirEvent(dir);
      }
    } else {
      pendingDir = DIR_CENTER;

      bool repeatable = (dir == DIR_UP || dir == DIR_DOWN);

      if (repeatable && now - holdStartMs >= 500 && now - lastRepeatMs >= 130) {
        handleDirEvent(dir);
        lastRepeatMs = now;
      }
    }

    vTaskDelay(15 / portTICK_PERIOD_MS);
  }
}

void inputTask(void *param) {
  (void)param;
  inputController.task();
}
