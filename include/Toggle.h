#pragma once
#include "vex.h"

void motorizedToggleing ();
void passiveToggleing();
int toggleTask(void* competition);
bool activeToggle(vex::competition& competition, bool redAlliance);

// Enable after configuring the optical sensor and tuning passive-toggle motion.
// bool passiveToggle(bool redAlliance);
