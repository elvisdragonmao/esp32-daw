#include "DisplayUI.h"

#include <SPI.h>

#include "Config.h"
#include "State.h"

DisplayUI displayUI;

DisplayUI::DisplayUI() : tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST) {
}

void DisplayUI::begin() {
  SPI.begin(PIN_TFT_SCLK, -1, PIN_TFT_MOSI, PIN_TFT_CS);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  COL_BG = ST77XX_BLACK;
  COL_PANEL = tft.color565(8, 35, 45);
  COL_PANEL_DARK = tft.color565(4, 20, 28);
  COL_TEXT = tft.color565(220, 245, 255);
  COL_MUTED_TEXT = tft.color565(120, 140, 145);
  COL_SELECT = tft.color565(255, 150, 0);
  COL_GRID_EMPTY = tft.color565(25, 90, 115);
  COL_GRID_BORDER = tft.color565(70, 150, 170);
  COL_NOTE = tft.color565(80, 210, 240);
  COL_MUTED_NOTE = tft.color565(70, 80, 85);
  COL_PLAYHEAD = ST77XX_WHITE;
  COL_RECORD = ST77XX_RED;

  tft.fillScreen(COL_BG);
  tft.setTextWrap(false);
}

void DisplayUI::drawMenuItem(uint8_t row, const char *text, bool selected, bool muted) {
  int y = VIEW_Y + row * 10;

  uint16_t bg = selected ? COL_SELECT : COL_PANEL;
  uint16_t fg = muted ? COL_MUTED_TEXT : COL_TEXT;

  tft.fillRect(0, y, MENU_W, 10, bg);
  tft.setTextSize(1);
  tft.setTextColor(fg, bg);
  tft.setCursor(2, y + 1);
  tft.print(text);
}

void DisplayUI::drawLeftPanel() {
  tft.fillRect(VIEW_X, VIEW_Y, MENU_W, VIEW_H, COL_PANEL);

  ScreenMode mode = screenMode;

  if (mode == MODE_MAIN) {
    char buf[12];

    for (uint8_t t = 0; t < TRACK_COUNT; t++) {
      snprintf(buf, sizeof(buf), "%d %s", t + 1, oscName(tracks[t].osc));
      drawMenuItem(t, buf, mainIndex == t, tracks[t].mute);
    }

    drawMenuItem(4, playing ? "Pause" : "Play", mainIndex == 4);
    drawMenuItem(5, "Vol", mainIndex == 5);
    drawMenuItem(6, "BPM", mainIndex == 6);
  }

  else if (mode == MODE_TRACK) {
    drawMenuItem(0, tracks[selectedTrack].mute ? "Unmut" : "Mute", trackMenuIndex == 0);
    drawMenuItem(1, "Record", trackMenuIndex == 1);
    drawMenuItem(2, "Vol", trackMenuIndex == 2);
    drawMenuItem(3, "OSC", trackMenuIndex == 3);

    char buf[10];
    snprintf(buf, sizeof(buf), "T%d", selectedTrack + 1);
    drawMenuItem(6, buf, false);
  }

  else if (mode == MODE_MASTER_VOL) {
    drawMenuItem(0, "M Vol", true);
    drawMenuItem(2, "Up +", false);
    drawMenuItem(3, "Dn -", false);
    drawMenuItem(6, "L Back", false);
  }

  else if (mode == MODE_TRACK_VOL) {
    char buf[10];
    snprintf(buf, sizeof(buf), "T%dVol", selectedTrack + 1);

    drawMenuItem(0, buf, true);
    drawMenuItem(2, "Up +", false);
    drawMenuItem(3, "Dn -", false);
    drawMenuItem(6, "L Back", false);
  }

  else if (mode == MODE_BPM) {
    drawMenuItem(0, "BPM", true);
    drawMenuItem(2, "Up +", false);
    drawMenuItem(3, "Dn -", false);
    drawMenuItem(6, "L Back", false);
  }

  else if (mode == MODE_OSC) {
    drawMenuItem(0, "Sin", oscMenuIndex == 0);
    drawMenuItem(1, "Tri", oscMenuIndex == 1);
    drawMenuItem(2, "Sqr", oscMenuIndex == 2);
    drawMenuItem(3, "Saw", oscMenuIndex == 3);

    char buf[10];
    snprintf(buf, sizeof(buf), "T%dOSC", selectedTrack + 1);
    drawMenuItem(6, buf, false);
  }

  else if (mode == MODE_REC_ARMED) {
    drawMenuItem(0, "ARM", true);
    char buf[10];
    snprintf(buf, sizeof(buf), "T%d", recordTrack + 1);
    drawMenuItem(1, buf, false);
    drawMenuItem(3, "Wait", false);
  }

  else if (mode == MODE_RECORDING) {
    drawMenuItem(0, "REC", true);
    char buf[10];
    snprintf(buf, sizeof(buf), "T%d", recordTrack + 1);
    drawMenuItem(1, buf, false);

    snprintf(buf, sizeof(buf), "%02d/16", recordCount + 1);
    drawMenuItem(3, buf, false);
  }
}

void DisplayUI::drawGridCell(uint8_t t, uint8_t s) {
  if (t >= TRACK_COUNT || s >= STEP_COUNT) return;

  int x = GRID_X + s * CELL_W;
  int y = GRID_Y + t * ROW_H;

  bool hasNote = tracks[t].steps[s].note >= 0;
  bool muted = tracks[t].mute;

  uint16_t fillColor;

  if (hasNote && muted) {
    fillColor = COL_MUTED_NOTE;
  } else if (hasNote) {
    fillColor = COL_NOTE;
  } else {
    fillColor = COL_GRID_EMPTY;
  }

  tft.fillRect(x, y, CELL_W - 1, CELL_H, fillColor);
  tft.drawRect(x, y, CELL_W - 1, CELL_H, COL_GRID_BORDER);

  if (playing && s == currentStep) {
    tft.drawRect(x - 1, y - 1, CELL_W + 1, CELL_H + 2, COL_PLAYHEAD);
  }
}

void DisplayUI::drawSelectedTrackOutline() {
  ScreenMode mode = screenMode;

  if (
    mode == MODE_TRACK || mode == MODE_TRACK_VOL || mode == MODE_OSC || mode == MODE_REC_ARMED || mode == MODE_RECORDING) {
    uint8_t t = selectedTrack;

    if (mode == MODE_REC_ARMED || mode == MODE_RECORDING) {
      t = recordTrack;
    }

    int y = GRID_Y + t * ROW_H;
    tft.drawRect(GRID_X - 2, y - 2, CELL_W * STEP_COUNT + 3, CELL_H + 4, COL_SELECT);
  }
}

void DisplayUI::drawGrid() {
  tft.fillRect(GRID_X - 2, VIEW_Y, VIEW_W - GRID_X + 2, STATUS_Y - VIEW_Y - 1, COL_BG);

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      drawGridCell(t, s);
    }
  }

  drawSelectedTrackOutline();
}

void DisplayUI::drawStepColumn(uint8_t s) {
  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    drawGridCell(t, s);
  }
  drawSelectedTrackOutline();
}

void DisplayUI::drawBar(int x, int y, int w, int h, int value, int maxValue) {
  value = clampInt(value, 0, maxValue);

  int fillW = (w * value) / maxValue;

  tft.drawRect(x, y, w, h, COL_GRID_BORDER);
  tft.fillRect(x + 1, y + 1, w - 2, h - 2, COL_GRID_EMPTY);

  if (fillW > 2) {
    tft.fillRect(x + 1, y + 1, fillW - 2, h - 2, COL_SELECT);
  }
}

void DisplayUI::drawStatus() {
  tft.fillRect(GRID_X - 2, STATUS_Y, VIEW_W - GRID_X + 2, STATUS_H, COL_PANEL_DARK);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_PANEL_DARK);

  char buf[32];
  ScreenMode mode = screenMode;

  if (mode == MODE_MASTER_VOL) {
    tft.setCursor(GRID_X, STATUS_Y + 2);
    tft.print("Master Volume");
    drawBar(GRID_X, STATUS_Y + 13, 100, 8, masterVolume, 127);

    snprintf(buf, sizeof(buf), "%03d", masterVolume);
    tft.setCursor(GRID_X + 104, STATUS_Y + 13);
    tft.print(buf);
    return;
  }

  if (mode == MODE_TRACK_VOL) {
    tft.setCursor(GRID_X, STATUS_Y + 2);
    snprintf(buf, sizeof(buf), "T%d Volume", selectedTrack + 1);
    tft.print(buf);

    drawBar(GRID_X, STATUS_Y + 13, 100, 8, tracks[selectedTrack].volume, 127);

    snprintf(buf, sizeof(buf), "%03d", tracks[selectedTrack].volume);
    tft.setCursor(GRID_X + 104, STATUS_Y + 13);
    tft.print(buf);
    return;
  }

  if (mode == MODE_BPM) {
    tft.setCursor(GRID_X, STATUS_Y + 2);
    tft.print("BPM");

    tft.setCursor(GRID_X, STATUS_Y + 13);
    tft.setTextColor(COL_SELECT, COL_PANEL_DARK);
    tft.print(bpm);
    return;
  }

  if (mode == MODE_RECORDING || mode == MODE_REC_ARMED) {
    int8_t n = currentRecNote;

    tft.setCursor(GRID_X, STATUS_Y + 2);

    if (mode == MODE_REC_ARMED) {
      tft.print("ARM: wait loop");
    } else {
      snprintf(buf, sizeof(buf), "REC T%d %02d/16", recordTrack + 1, recordCount + 1);
      tft.print(buf);
    }

    tft.setCursor(GRID_X, STATUS_Y + 13);

    if (n >= 0) {
      snprintf(buf, sizeof(buf), "Note: %s", NOTE_NAMES[n]);
    } else {
      snprintf(buf, sizeof(buf), "Note: --");
    }

    tft.print(buf);
    return;
  }

  tft.setCursor(GRID_X, STATUS_Y + 2);
  snprintf(buf, sizeof(buf), "BPM%03d V%03d S%02d", bpm, masterVolume, currentStep + 1);
  tft.print(buf);

  tft.setCursor(GRID_X, STATUS_Y + 13);

  if (playing) {
    tft.print("PLAY");
  } else {
    tft.print("STOP");
  }

  tft.print("  ");

  if (mode == MODE_MAIN) {
    tft.print("MAIN");
  } else if (mode == MODE_TRACK) {
    snprintf(buf, sizeof(buf), "T%d MENU", selectedTrack + 1);
    tft.print(buf);
  } else if (mode == MODE_OSC) {
    tft.print("OSC");
  }
}

void DisplayUI::drawFullScreen() {
  tft.fillScreen(COL_BG);
  drawLeftPanel();
  drawGrid();
  drawStatus();
}

void DisplayUI::drawDirtyUI() {
  bool localFull;
  bool localMenu;
  bool localStatus;
  bool localCol[STEP_COUNT];
  bool localCell[TRACK_COUNT][STEP_COUNT];

  portENTER_CRITICAL(&stateMux);

  localFull = dirtyFull;
  localMenu = dirtyMenu;
  localStatus = dirtyStatus;

  for (uint8_t s = 0; s < STEP_COUNT; s++) {
    localCol[s] = dirtyCol[s];
  }

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      localCell[t][s] = dirtyCell[t][s];
    }
  }

  dirtyFull = false;
  dirtyMenu = false;
  dirtyStatus = false;

  for (uint8_t s = 0; s < STEP_COUNT; s++) {
    dirtyCol[s] = false;
  }

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      dirtyCell[t][s] = false;
    }
  }

  portEXIT_CRITICAL(&stateMux);

  if (localFull) {
    drawFullScreen();
    return;
  }

  if (localMenu) {
    drawLeftPanel();
  }

  bool gridChanged = false;

  for (uint8_t s = 0; s < STEP_COUNT; s++) {
    if (localCol[s]) {
      drawStepColumn(s);
      gridChanged = true;
    }
  }

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      if (localCell[t][s]) {
        drawGridCell(t, s);
        gridChanged = true;
      }
    }
  }

  if (gridChanged) {
    drawSelectedTrackOutline();
  }

  if (localStatus) {
    drawStatus();
  }
}

void DisplayUI::task() {
  while (true) {
    drawDirtyUI();
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }
}

void uiTask(void *param) {
  (void)param;
  displayUI.task();
}
