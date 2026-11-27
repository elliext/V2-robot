# pragma once
# include <vex.h>

// Initial match setup: R1 = Right wing, L1 = Left wing; Up = Red, Down = Blue.
// A confirms. Left/Right directional arrows are unused.
void selectMatchSetup();

// Shared by autonomous and driver control.
extern bool redAlliance;
// Viewed from our alliance station looking into the field.
extern bool isRight;
