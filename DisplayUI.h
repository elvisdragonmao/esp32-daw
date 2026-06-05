#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Arduino.h>
#include <Adafruit_ST7735.h>

#include "Types.h"

class DisplayUI {
public:
  DisplayUI();
  void begin();
  void task();

private:
  Adafruit_ST7735 tft;

  void applyTftColorOrder();
  uint8_t gridStepCount();
  uint8_t gridCellWidth(uint8_t stepCount);
  void drawOscShape(int x, int y, OscType osc, uint16_t color);
  void drawOscMenuItem(uint8_t row, OscType osc, bool selected);
  void drawTrackOscItem(uint8_t row, uint8_t track, bool selected, bool muted);
  void drawMenuItem(uint8_t row, const char *text, bool selected, bool muted = false);
  void drawLeftPanel();
  void drawGridCell(uint8_t t, uint8_t s);
  void drawSelectedTrackOutline();
  void drawGrid();
  void drawStepColumn(uint8_t s);
  void drawBar(int x, int y, int w, int h, int value, int maxValue);
  void drawStatus();
  void drawFullScreen();
  void drawDirtyUI();
};

extern DisplayUI displayUI;

void uiTask(void *param);

#endif
