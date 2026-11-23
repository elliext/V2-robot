#pragma once

#include "vex.h"
#include "robot-config.h"

// Drive target and tolerance are in inches.
void drivePID (double target, double maxIntegral = 500, double tolerance = 0.5);
void turnPID(double target, double maxIntegral= 500, double tolerance = 0.5);
void armPID(double target, double maxIntegral = 500, double tolerance = 0.5);
