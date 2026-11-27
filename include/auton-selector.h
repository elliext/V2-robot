# pragma once
# include <vex.h>

// Pre-auton controls: R1 = Right wing, L1 = Left wing; Up = Red, Down = Blue.
// A confirms. Left/Right directional arrows are unused.
void selectAuton();

extern bool redAlliance;
// Viewed from our alliance station looking into the field.
extern bool isRight;
