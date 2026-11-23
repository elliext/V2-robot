#include "vex.h"
#include "Arm.h"

using namespace vex;

namespace {
  bool previousPressed = false;
  bool presetActive = false;
  bool presetMoving = false;
  timer presetTimer;
}

void ArmControll(bool resetHold){
  static bool holdingPosition = false;
  static double leftHoldPosition = 0;
  static double rightHoldPosition = 0;
  const double kP = 2.0;
  const double MAX_HOLD_SPEED = 1.0;

  if(resetHold){
    holdingPosition = false;
    previousPressed = false;
    presetActive = false;
    presetMoving = false;
  }
  if(holdingToggle()){
    holdingPosition = false;
    return;
  }
  if(Controller.ButtonL1.pressing()){
    holdingPosition = false;
    Arm.spin(reverse, 100, percent);
  }
  else if(Controller.ButtonL2.pressing()){
    holdingPosition = false;
    Arm.spin(forward, 100, percent);
  }
  else if (Controller.ButtonB.pressing()){
    holdingPosition = false;
    Arm.spin(reverse,25,percent);
  }
  else{
    const double leftPosition = ArmLeft.position(degrees);
    const double rightPosition = ArmRight.position(degrees);
    if(!holdingPosition){
      leftHoldPosition = leftPosition;
      rightHoldPosition = rightPosition;
      holdingPosition = true;
    }

    // P-only feedback holds each arm using its own encoder without waiting.
    const double leftSpeed = kP * (leftHoldPosition - leftPosition);
    const double rightSpeed = kP * (rightHoldPosition - rightPosition);
    ArmLeft.spin(forward,fmax(-MAX_HOLD_SPEED,fmin(MAX_HOLD_SPEED,leftSpeed)),percent);
    ArmRight.spin(forward,fmax(-MAX_HOLD_SPEED,fmin(MAX_HOLD_SPEED,rightSpeed)),percent);
  }
}

bool holdingToggle(){
  const double PRESET_TIMEOUT_MS = 3000;
  const bool pressed = Controller.ButtonA.pressing();
  const bool startPreset = pressed && !previousPressed;
  previousPressed = pressed;

  if(Controller.ButtonL1.pressing() || Controller.ButtonL2.pressing()){
    if(presetActive){
      Arm.stop(brake);
    }
    presetActive = false;
    presetMoving = false;
    return false;
  }

  if(startPreset){
    presetTimer.reset();
    ArmLeft.spinToPosition(-30,degrees,false);
    ArmRight.spinToPosition(30,degrees,false);
    presetActive = true;
    presetMoving = true;
  }

  if(presetMoving){
    const bool finished = ArmLeft.isDone() && ArmRight.isDone();
    if(finished || presetTimer.time(msec) >= PRESET_TIMEOUT_MS){
      Arm.stop(hold);
      presetMoving = false;
    }
  }
  return presetActive;
}

void ArmGoodSetting(){
  Arm.spin(reverse,100,percent);
  wait(3500,msec);
  claw.spinToPosition(-100,degrees, 50, velocityUnits::pct);
  wait(500,msec);
  Arm.stop();
  Arm.spin(forward,100,percent);
}

int armTask(void* competition){
  auto& competitionState = *static_cast<vex::competition*>(competition);
  bool resetHold = true;
  while(competitionState.isEnabled() && competitionState.isDriverControl()){
    ArmControll(resetHold);
    resetHold = false;
    wait(20, msec);
  }
  return 0;
}
