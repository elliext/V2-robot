#pragma once

void move(double speedPercent);
void turn(double angleDegrees);
void updateDrivetrain();
void arcadeDrive();
int drivetrainTask(void* competition);
