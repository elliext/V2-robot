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

void scoreLeftAllianceGoal(void) {
  pickUpPin();
  drivePID(-10, 20, 1);
  Arm.spin(forward, 85, percent);
  wait(650, msec);
  claw.spinFor(reverse, 600 , msec, 300, rpm);
}

void scoreRightAllianceGoal(void) {
  pickUpPin();
  drivePID(-10, 20, 1);
  Arm.spin(forward, 85, percent);
  wait(650, msec);
  claw.spinFor(reverse, 600 , msec, 300, rpm);
}
void leftWing(void) {
  if (!activeToggle()) {
    return;
  }
  turnPID(-75, 20, 0.25);
  Arm.spinFor(forward, 200, deg, 300, rpm, false);
  drivePID(15, 20, 0.8);
}

void rightWing(void) {
  Drive.stop(brake);
  if (!activeToggle()) {
    return;
  }
  turnPID(75, 20, 0.25);
  Arm.spinFor(forward, 200, deg, 300, rpm, false);
  drivePID(15, 20, 0.8);
}
//Spin for sintax:
// bool spinFor(
//  directionType dir,
//  double        rotation,
//  rotationUnits units,
//  double        velocity,
//  velocityUnits units_v,
//  bool          waitForCompletion = true );

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
  switch(selectedWing) {
    case AutonWing::Right:
      rightWing();
      break;
    case AutonWing::Left:
      leftWing();
      break;
    default:
      Drive.stop(brake);
      printf("Autonomous: invalid wing selection %d.\n", static_cast<int>(selectedWing));
      Controller.rumble("---");
      return;
  }
}
