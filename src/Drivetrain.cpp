#include "vex.h"
#include "Drivetrain.h"

using namespace vex;

namespace {
double requestedSpeed = 0;
double requestedAngle = 0;
bool precisionMode = false;
bool previousBPressed = false;
}

void move(double speedPercent){
  if (!isfinite(speedPercent)){
    printf("Drivetrain: move speed must be finite; stopping.\n");
    requestedSpeed = 0;
    updateDrivetrain();
    return;
  }
  requestedSpeed = fmax(-100, fmin(100, speedPercent));
}

void turn(double angleDegrees){
  if (!isfinite(angleDegrees)){
    printf("Drivetrain: turn angle must be finite; stopping.\n");
    requestedSpeed = 0;
    requestedAngle = 0;
    updateDrivetrain();
    return;
  }
  requestedAngle = fmod(angleDegrees, 360);
}

void updateDrivetrain(){
  const double angleRadians = requestedAngle * M_PI / 180;
  const double straightVelocity = requestedSpeed * cos(angleRadians);
  const double turnVelocity = requestedSpeed * sin(angleRadians) * 0.7;
  double leftVelocity = straightVelocity + turnVelocity;
  double rightVelocity = straightVelocity - turnVelocity;
  const double peakVelocity = fmax(fabs(leftVelocity), fabs(rightVelocity));
  if (peakVelocity > 100){
    leftVelocity *= 100 / peakVelocity;
    rightVelocity *= 100 / peakVelocity;
  }
  const double speedScale = precisionMode ? 0.5 : 0.8;
  leftVelocity *= speedScale;
  rightVelocity *= speedScale;
  LeftDrive.spin(reverse, leftVelocity, percent);
  RightDrive.spin(reverse, rightVelocity, percent);
}

void arcadeDrive(){
  const bool bPressed = Controller.ButtonB.pressing();
  if(bPressed && !previousBPressed){
    precisionMode = !precisionMode;
  }
  previousBPressed = bPressed;

  double forwardInput = Controller.Axis3.position();
  double steeringInput = Controller.Axis1.position();
  if (fabs(forwardInput) < 5){
    forwardInput = 0;
  }
  if (fabs(steeringInput) < 5){
    steeringInput = 0;
  }
  const double magnitude = fmin(100, sqrt(forwardInput * forwardInput +
                                         steeringInput * steeringInput));
  const double angleDegrees = magnitude == 0 ? 0 :
      atan2(steeringInput, forwardInput) * 180 / M_PI;
  move(magnitude);
  turn(angleDegrees);
  updateDrivetrain();
}

int drivetrainTask(void* competition){
  auto& competitionState = *static_cast<vex::competition*>(competition);
  requestedSpeed = 0;
  requestedAngle = 0;
  while(competitionState.isEnabled() && competitionState.isDriverControl()){
    arcadeDrive();
    wait(20, msec);
  }
  return 0;
}
