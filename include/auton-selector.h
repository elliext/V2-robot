# pragma once
# include <vex.h>

// Startup controls: Up = Red, Down = Blue. Shoulder buttons are unused.
// Left = Left Wing, Right = Right Wing; A confirms.
// Brain touchscreen: tap wing/color buttons, then Confirm.
void selectAuton();

// Shared by autonomous and driver control.
enum class AutonWing { Right, Left };
extern AutonWing selectedWing;
extern bool redAlliance;
