#include "vex.h"
#include "Toggle.h"
#include "auton-selector.h"

using namespace vex;

namespace {
void showOpticalSensor(optical& sensor, int line, const char* side){
  Controller.Screen.clearLine(line);
  Controller.Screen.setCursor(line,1);
  if(!sensor.installed()){
    Controller.Screen.print("%s: sensor missing", side);
    return;
  }
  Controller.Screen.print("%s H:%.1f Near:%s", side, sensor.hue(),
                          sensor.isNearObject() ? "Y" : "N");
}

bool updateActiveSide(motor& toggleMotor, optical& sensor, bool redAlliance,
                      bool completed){
  if(completed || !sensor.isNearObject()){
    toggleMotor.stop(coast);
    return completed;
  }
  const double hue = sensor.hue();
  const bool targetColor = redAlliance ? (hue < 40 || hue >= 350) :
                                        (hue >= 100 && hue <= 260);
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
  toggleOptical1.setLight(ledState::off);
  toggleOptical2.setLight(ledState::off);
}

void enableToggleLights(){
  toggleOptical1.setLightPower(100,percent);
  toggleOptical2.setLightPower(100,percent);
  toggleOptical1.setLight(ledState::on);
  toggleOptical2.setLight(ledState::on);
}
}

void updateOpticalDisplay(){
  static timer displayTimer;
  static bool firstUpdate = true;
  if(!firstUpdate && displayTimer.time(msec) < 100){
    return;
  }
  firstUpdate = false;
  displayTimer.reset();
  showOpticalSensor(toggleOptical2,1,"L");
  showOpticalSensor(toggleOptical1,2,"R");
}

bool activeToggle(vex::competition& competitionState, bool redAlliance){
  const double TIMEOUT_MS = 3000;
  stopActiveToggle();
  if(!competitionState.isEnabled() || !competitionState.isAutonomous()){
    printf("Active toggle: requires enabled autonomous mode.\n");
    return false;
  }
  if(!toggleOptical1.installed() || !toggleOptical2.installed()){
    printf("Active toggle: optical sensor missing on port 5 or 6.\n");
    Controller.rumble("---");
    return false;
  }
  enableToggleLights();
  timer toggleTimer;
  toggleTimer.reset();
  bool leftCompleted = false;
  bool rightCompleted = false;
  while(competitionState.isEnabled() && competitionState.isAutonomous() &&
        toggleTimer.time(msec) < TIMEOUT_MS){
    if(!toggleOptical1.installed() || !toggleOptical2.installed()){
      stopActiveToggle();
      printf("Active toggle: optical sensor disconnected.\n");
      Controller.rumble("---");
      return false;
    }
    leftCompleted = updateActiveSide(toggleLeft, toggleOptical2,
                                    redAlliance, leftCompleted);
    rightCompleted = updateActiveSide(toggleRight, toggleOptical1,
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
  // Passive toggle skeleton only; no optical sensor is configured yet.

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

void motorizedToggleing(bool resetHold){
  static bool opticalActive = false;
  static bool leftCompleted = false;
  static bool rightCompleted = false;
  static bool sensorFault = false;
  if(resetHold || !Controller.ButtonA.pressing() ||
     Controller.ButtonX.pressing()){
    opticalActive = false;
    leftCompleted = false;
    rightCompleted = false;
    sensorFault = false;
    stopActiveToggle();
  }
  if(resetHold){
    return;
  }
  if (Controller.ButtonX.pressing()){
    toggleLeft.spin(reverse,100,percent);
    toggleRight.spin(reverse,100,percent);
  }
  else if(Controller.ButtonA.pressing()){
    if(sensorFault){
      return;
    }
    if(!toggleOptical1.installed() || !toggleOptical2.installed()){
      stopActiveToggle();
      sensorFault = true;
      printf("Driver active toggle: optical sensor missing on port 5 or 6; release A to retry.\n");
      Controller.rumble("---");
      return;
    }
    if(!opticalActive){
      enableToggleLights();
      opticalActive = true;
    }
    leftCompleted = updateActiveSide(toggleLeft, toggleOptical2,
                                    redAlliance, leftCompleted);
    rightCompleted = updateActiveSide(toggleRight, toggleOptical1,
                                     redAlliance, rightCompleted);
  }
}


int toggleTask(void* competition){
  auto& competitionState = *static_cast<vex::competition*>(competition);
  motorizedToggleing(true);
  while(competitionState.isEnabled() && competitionState.isDriverControl()){
    motorizedToggleing();
    wait(20, msec);
  }
  motorizedToggleing(true);
  return 0;
}
