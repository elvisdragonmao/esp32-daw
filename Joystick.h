#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <Arduino.h>

#include "Types.h"

class Joystick {
public:
  void begin();
  void calibrate();
  void readDelta(int &dx, int &dy);
  Dir4 readDir4();
  int8_t readRecordNote8();

private:
  int centerX = 2048;
  int centerY = 2048;
  int filteredX = 2048;
  int filteredY = 2048;

  int readPinAverage(uint8_t pin);
  int clampAdc(int value);
  int normalizeAxis(int value, int center);
};

extern Joystick joystick;

#endif
