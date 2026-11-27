#pragma once

void move(double speedPercent);
void turn(double angleDegrees);
void updateDrivetrain();
void arcadeDrive();
// Arcade inputs, -100..100; positive steering turns clockwise.
void driveInputs(double forwardInput, double steeringInput);
int drivetrainTask(void* competition);
