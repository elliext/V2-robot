#pragma once

#include "vex.h"

class Claw {
public:
  explicit Claw(vex::motor& motor);

  void open();
  void close();
  void stop();

private:
  vex::motor& motor_;
};

void clawControll(bool resetHold = false);
void clawOpen();
void clawClose();
void manualClawOpen();
void manualClawClose();
void clawHold();
int clawTask(void* competition);
