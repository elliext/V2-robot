#pragma once
#include "vex.h"

void motorizedToggleing(bool resetHold = false);
void passiveToggleing();
int toggleTask(void* competition);
bool activeToggle(vex::competition& competition, bool redAlliance);
void updateOpticalDisplay();

// Enable after configuring the optical sensor and tuning passive-toggle motion.
// bool passiveToggle(bool redAlliance);
