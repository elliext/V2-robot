#include "vex.h"
#include "Toggle.h"

using namespace vex;

namespace {
bool updateActiveSide(motor& toggleMotor, optical& sensor, bool redAlliance,
                      bool completed){
  if(completed || !sensor.isNearObject()){
    toggleMotor.stop(coast);
    return completed;
  }
  const double hue = sensor.hue();
  const bool targetColor = redAlliance ? (hue < 30 || hue > 330) :
                                        (hue >= 180 && hue <= 260);
  if(targetColor){
    toggleMotor.stop(coast);
    return true;
  }
  toggleMotor.spin(reverse,100,percent);
  return false;
}

void stopActiveToggle(){
  toggleLeft.stop(coast);
  toggleRight.stop(coast);
  toggleOpticalLeft.setLight(ledState::off);
  toggleOpticalRight.setLight(ledState::off);
}
}

bool activeToggle(vex::competition& competitionState, bool redAlliance){
  const double TIMEOUT_MS = 3000;
  stopActiveToggle();
  if(!competitionState.isEnabled() || !competitionState.isAutonomous()){
    printf("Active toggle: requires enabled autonomous mode.\n");
    return false;
  }
  if(!toggleOpticalLeft.installed() || !toggleOpticalRight.installed()){
    printf("Active toggle: optical sensor missing on port 5 or 6.\n");
    Controller.rumble("---");
    return false;
  }
  toggleOpticalLeft.setLightPower(50,percent);
  toggleOpticalRight.setLightPower(50,percent);
  toggleOpticalLeft.setLight(ledState::on);
  toggleOpticalRight.setLight(ledState::on);
  timer toggleTimer;
  toggleTimer.reset();
  bool leftCompleted = false;
  bool rightCompleted = false;
  while(competitionState.isEnabled() && competitionState.isAutonomous() &&
        toggleTimer.time(msec) < TIMEOUT_MS){
    if(!toggleOpticalLeft.installed() || !toggleOpticalRight.installed()){
      stopActiveToggle();
      printf("Active toggle: optical sensor disconnected.\n");
      Controller.rumble("---");
      return false;
    }
    leftCompleted = updateActiveSide(toggleLeft, toggleOpticalLeft,
                                    redAlliance, leftCompleted);
    rightCompleted = updateActiveSide(toggleRight, toggleOpticalRight,
                                     redAlliance, rightCompleted);
    if(leftCompleted && rightCompleted){
      stopActiveToggle();
      return true;
    }
    wait(20,msec);
  }
  stopActiveToggle();
  printf("Active toggle: %s; left %s, right %s.\n",
         competitionState.isEnabled() && competitionState.isAutonomous() ?
             "timed out" : "autonomous ended",
         leftCompleted ? "complete" : "incomplete",
         rightCompleted ? "complete" : "incomplete");
  Controller.rumble("---");
  return false;
}

void passiveToggleing(){
  // Passive drivetrain-push example remains inactive.

  // TODO: Declare/configure an optical sensor named toggleOptical in robot-config.
  // TODO: Tune hue ranges, lighting, push speed, and timeout on the robot.
  // Call only from autonomous, with no other task commanding the drivetrain.
  //
  // bool passiveToggle(bool redAlliance) {
  //   const double pushSpeedPercent = 20;
  //   const double timeoutMs = 3000;
  //   timer pushTimer;
  //   toggleOptical.setLightPower(50, percent);
  //   toggleOptical.setLight(ledState::on);
  //   pushTimer.reset();
  //
  //   while (pushTimer.time(msec) < timeoutMs) {
  //     const double hue = toggleOptical.hue();
  //     const bool seesRed = hue < 30 || hue > 330;
  //     const bool seesBlue = hue >= 180 && hue <= 260;
  //     const bool matchesAlliance = redAlliance ? seesRed : seesBlue;
  //     if (toggleOptical.isNearObject() && matchesAlliance) {
  //       Drive.stop(brake);
  //       toggleOptical.setLight(ledState::off);
  //       return true;
  //     }
  //
  //     Push the passive mechanism with the drivetrain, not the toggle motor.
  //     Drive.spin(reverse, pushSpeedPercent, percent);
  //     wait(20, msec);
  //   }
  //
  //   Drive.stop(brake);
  //   toggleOptical.setLight(ledState::off);
  //   printf("Passive toggle: timed out before detecting alliance color.\n");
  //   return false;
}

void motorizedToggleing(){
  if (Controller.ButtonX.pressing()){
    toggleLeft.spin(reverse,100,percent);
    toggleRight.spin(reverse,100,percent);
  }
  else{
    toggleLeft.stop(coast);
    toggleRight.stop(coast);
  }
}


int toggleTask(void* competition){
  auto& competitionState = *static_cast<vex::competition*>(competition);
  while(competitionState.isEnabled() && competitionState.isDriverControl()){
    motorizedToggleing();
    wait(20, msec);
  }
  return 0;
}
