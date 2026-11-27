#include "vex.h"
#include "PID.h"
#include "auton-selector.h"
#include "autonomos.h"
#include "Toggle.h"

using namespace vex;
extern vex::competition Competition;

bool activeToggle(){
  return activeToggle(Competition, redAlliance);
}

void pickUpPin(void) {
  claw.spinFor(forward, 800 , msec, 300, rpm);
  claw.stop(hold);
  Arm.spin(reverse, 90, percent);
  wait(450, msec);
  Arm.stop(brake);
  
}

void scoreAllianceGoal(void) {
  pickUpPin();
  drivePID(-10, 20, 1);
  Arm.spin(forward, 85, percent);
  wait(650, msec);
  claw.spinFor(reverse, 600 , msec, 300, rpm);
}
namespace {
void passiveToggleWing(double turnDirection) {
  drivePID(20, 20, 0.8);
  turnPID(82 * turnDirection, 20, 0.5);
  drivePID(12, 50, 0.5);
  drivePID(-6, 20, 0.5);
  drivePID(10, 60, 0.5);
  drivePID(-20, 20, 0.5);
  turnPID(42.5 * turnDirection, 20, 0.3);
}

void scoreAndToggleWing() {
  // Swap the wing sign mapping here if field testing disagrees.
  const double turnDirection = isRight ? 1.0 : -1.0;
  scoreAllianceGoal();
  turnPID(-60 * turnDirection, 20, 2);
  claw.spinFor(forward, 150, msec, 300, rpm);
  passiveToggleWing(turnDirection);
  Arm.stop(brake);
}
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              Autonomous Task                              */
/*                                                                           */
/*  This task is used to control your robot during the autonomous phase of   */
/*  a VEX Competition.                                                       */
/*                                                                           */
/*  You must modify the code to add your own robot specific commands here.   */
/*---------------------------------------------------------------------------*/

void autonomous(void) {
  scoreAndToggleWing();
}
