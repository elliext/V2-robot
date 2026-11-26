#include "vex.h"
#include "Toggle.h"
#include "auton-selector.h"
#include <initializer_list>

using namespace vex;
namespace {
// ---- Optical tuning. Measured on robot: yellow 21-42, blue OK in 180-260.
// Verify red with the controller readout and move kRedHi/kRedLo if needed. ----
constexpr double kLightPowerPct      = 100;   // 100% washes out glossy plastic
constexpr int    kProximityThreshold = 40;   // 0-255; lower = triggers farther away
constexpr double kRedLo    = 360, kRedHi    = 24;   // wraps around 0
constexpr double kYellowLo = 24,  kYellowHi = 70;
constexpr double kBlueLo   = 100, kBlueHi   = 330;


bool isRed(double h)    { return h >= kRedLo || h <= kRedHi; }
bool isYellow(double h) { return h >= kYellowLo && h <= kYellowHi; }
bool isBlue(double h)   { return h >= kBlueLo && h <= kBlueHi; }


const char* colorName(double h){
  if(isRed(h))    return "R";
  if(isBlue(h))   return "B";
  if(isYellow(h)) return "Y";
  return "?";
}


void showOpticalSensor(optical& sensor, int line, const char* side){
  Controller.Screen.clearLine(line);
  Controller.Screen.setCursor(line,1);
  if(!sensor.installed()){
    Controller.Screen.print("%s: sensor missing", side);
    return;
  }
  const double hue = sensor.hue();
  Controller.Screen.print("%s H:%3.0f %s Near:%s", side, hue, colorName(hue),
                          sensor.isNearObject() ? "Y" : "N");
}


bool updateActiveSide(motor& toggleMotor, optical& sensor, bool redAlliance,
                      bool completed){
  if(completed){
    toggleMotor.stop(coast);
    return true;
  }
  const double hue = sensor.hue();
  const bool targetColor = redAlliance ? isRed(hue) : isBlue(hue);
  if(sensor.isNearObject() && targetColor){
    toggleMotor.stop(coast);
    return true;
  }
  // Keep turning until we SEE our color. Do not pause on proximity flicker;
  // the auton timeout / driver button release is the safety net.
  toggleMotor.spin(reverse,100,percent);
  return false;
}


void stopActiveToggle(){
  toggleLeft.stop(coast);
  toggleRight.stop(coast);
  // LEDs intentionally stay on so the controller readout matches the loop.
}
}  // namespace


void enableToggleLights(){
  static bool lightsOn = false;
  for(optical* s : {&toggleOptical1, &toggleOptical2}){
    s->objectDetectThreshold(kProximityThreshold);
    s->setLightPower(kLightPowerPct, percent);
    s->setLight(ledState::on);
  }
  if(!lightsOn){
    wait(150, msec);   // let one integration window finish with the LED on
    lightsOn = true;
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
    printf("Active toggle: optical sensor missing on port 5 or 15.\n");
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
      printf("Driver active toggle: optical sensor missing on port 5 or 15; release A to retry.\n");
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
