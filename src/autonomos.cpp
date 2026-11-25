#include "vex.h"
#include "PID.h"
#include "auton-selector.h"
#include "autonomos.h"
#include "Toggle.h"

using namespace vex;
extern vex::competition Competition;

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
void passiveToggleQuadrant1(void) {
  drivePID(20, 20, 0.8);
  turnPID(82, 20, 0.5);
  drivePID(12, 50, 0.5);
  drivePID(-6, 20, 0.5);
  drivePID(10, 60, 0.5);
  drivePID(-20, 20, 0.5);
  turnPID(-42.5, 20, 0.3);
}
void passiveToggleQuadrant2(void) {
  drivePID(20, 20, 0.8);
  turnPID(-82, 20, 0.5);
  drivePID(12, 50, 0.5);
  drivePID(-6, 20, 0.5);
  drivePID(10, 60, 0.5);
  drivePID(-20, 20, 0.5);
  turnPID(-42.5, 20, 0.3);
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
  switch(autonNumber){
    case 2:
      scoreAllianceGoal();
      turnPID(60, 20, 2);
      claw.spinFor(forward, 150 , msec, 300, rpm);
      passiveToggleQuadrant2();
      Arm.stop(brake);
    break;
    case 1:
      drivePID(-24, 20, 0.5);
      //task monitorToggle(monitorToggleCallback);
      break;
    case 3:
      //auton code
      break;
    case 4:
      break;
    case 5:
      break;
    case 6:
      Drive.stop(brake);
      activeToggle(Competition, redAlliance);
      break;
    case 0:
      scoreAllianceGoal();
      turnPID(-60, 20, 2);
      claw.spinFor(forward, 150 , msec, 300, rpm);
      passiveToggleQuadrant1();
      Arm.stop(brake);
      // drivePID(-25, 25, 0.5);
      break;
  }
  

  // ..........................................................................
  // Insert autonomous user code here.
  // ..........................................................................
}
