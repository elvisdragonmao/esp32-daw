#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include <Arduino.h>

#include "Types.h"

class InputController {
public:
  void task();

private:
  void handleDirEvent(Dir4 dir);
};

extern InputController inputController;

void inputTask(void *param);

#endif
