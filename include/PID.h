#pragma once

#include <vex.h>
#include "robot-config.h"

// Drive target and tolerance are in inches.
void drivePID (double target, double maxIntegral = 500, double tolerance = 0.5);
// Integral is capped at +/-maxIntegral; timeoutMs is nonnegative, with 0 disabling timeout.
// Positive targets turn clockwise; negative targets turn counterclockwise.
// Success requires 150 ms inside tolerance at <=5 degrees/second.
void turnPID(double target, double maxIntegral= 500, double tolerance = 0.5,
             double timeoutMs = 0);
void armPID(double target, double maxIntegral = 500, double tolerance = 0.5);
