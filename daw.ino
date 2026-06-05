#include <Arduino.h>

#include "AudioEngine.h"
#include "DisplayUI.h"
#include "InputController.h"
#include "Joystick.h"
#include "State.h"

static void clearDirtyFlags() {
  for (uint8_t s = 0; s < STEP_COUNT; s++) {
    dirtyCol[s] = false;
  }

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      dirtyCell[t][s] = false;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BOOT 1");

  Serial.println("BOOT 2 joystick");
  joystick.begin();

  Serial.println("BOOT 3 data");
  initSineTable();
  initTracks();
  clearDirtyFlags();

  Serial.println("BOOT 4 display");
  displayUI.begin();

  Serial.println("BOOT 5 I2S");
  audioEngine.begin();

  Serial.println("BOOT 6 tasks");

  xTaskCreatePinnedToCore(audioTask, "audioTask", 8192, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(inputTask, "inputTask", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(uiTask, "uiTask", 8192, NULL, 1, NULL, 0);

  Serial.println("BOOT DONE");
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
