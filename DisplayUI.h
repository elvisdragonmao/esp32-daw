#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Arduino.h>
#include <Adafruit_ST7735.h>

class DisplayUI {
public:
  DisplayUI();
  void begin();
  void task();

private:
  Adafruit_ST7735 tft;

  void applyTftColorOrder();
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
