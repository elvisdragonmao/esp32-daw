#include "Joystick.h"

Joystick joystick;

void Joystick::begin() {
  pinMode(PIN_JOY_SW, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_JOY_X, ADC_11db);
  analogSetPinAttenuation(PIN_JOY_Y, ADC_11db);

  calibrate();
}

void Joystick::calibrate() {
  delay(200);

  long sx = 0;
  long sy = 0;

  for (int i = 0; i < 100; i++) {
    sx += analogRead(PIN_JOY_X);
    sy += analogRead(PIN_JOY_Y);
    delay(2);
  }

  centerX = sx / 100;
  centerY = sy / 100;
  filteredX = centerX;
  filteredY = centerY;
}

int Joystick::readPinAverage(uint8_t pin) {
  long sum = 0;

  for (int i = 0; i < JOY_ANALOG_SAMPLES; i++) {
    sum += analogRead(pin);
  }

  return sum / JOY_ANALOG_SAMPLES;
}

void Joystick::readDelta(int &dx, int &dy) {
  int rawX = readPinAverage(PIN_JOY_X);
  int rawY = readPinAverage(PIN_JOY_Y);

  filteredX = ((filteredX * (JOY_FILTER_WEIGHT - 1)) + rawX) / JOY_FILTER_WEIGHT;
  filteredY = ((filteredY * (JOY_FILTER_WEIGHT - 1)) + rawY) / JOY_FILTER_WEIGHT;

  dx = filteredX - centerX;
  dy = filteredY - centerY;

  if (JOY_INVERT_X) dx = -dx;
  if (JOY_INVERT_Y) dy = -dy;
}

Dir4 Joystick::readDir4() {
  int dx, dy;
  readDelta(dx, dy);

  int ax = abs(dx);
  int ay = abs(dy);

  if (ax < JOY_NAV_DEAD && ay < JOY_NAV_DEAD) {
    return DIR_CENTER;
  }

  if (ax >= JOY_NAV_DEAD && ax > ay + JOY_AXIS_MARGIN) {
    return dx > 0 ? DIR_RIGHT : DIR_LEFT;
  }

  if (ay >= JOY_NAV_DEAD && ay > ax + JOY_AXIS_MARGIN) {
    return dy > 0 ? DIR_UP : DIR_DOWN;
  }

  return DIR_CENTER;
}

int8_t Joystick::readRecordNote8() {
  int dx, dy;
  readDelta(dx, dy);

  bool left = dx < -JOY_RECORD_DEAD;
  bool right = dx > JOY_RECORD_DEAD;
  bool up = dy > JOY_RECORD_DEAD;
  bool down = dy < -JOY_RECORD_DEAD;

  if (!left && !right && !up && !down) return -1;

  if (left && up) return 7;

  if (up && !left && !right) return 1;

  if (right && up) return 2;

  if (right && !up && !down) return 3;

  if (right && down) return 4;

  if (down && !left && !right) return 5;

  if (left && down) return 6;
  if (left && !up && !down) return 0;

  return -1;
}
