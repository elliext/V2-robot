#include "vex.h"
#include "Toggle.h"

using namespace vex;

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

void motorizedToggleing(){
  if (Controller.ButtonX.pressing()){
    toggle.spin(reverse,100,percent);
  }
  else{
    toggle.stop(coast);
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
