/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       VEX                                                       */
/*    Created:      Thu Sep 26 2019                                           */
/*    Description:  Competition Template                                      */
/*                                                                            */
/*----------------------------------------------------------------------------*/

// ---- START VEXCODE CONFIGURED DEVICES ----
// ---- END VEXCODE CONFIGURED DEVICES ----

#include "vex.h"
#include "autonomos.h"
#include "auton-selector.h"
#include "Claw.h"
#include "Arm.h"
#include "Drivetrain.h"
#include "Toggle.h"
using namespace vex;

// A global instance of competition                          
competition Competition;
// define your global instances of motors and other devices here

/*---------------------------------------------------------------------------*/
/*                          Pre-Autonomous Functions                         */
/*                                                                           */
/*  You may want to perform some actions before the competition starts.      */
/*  Do them in the following function.  You must return from this function   */
/*  or the autonomous and usercontrol tasks will not be started.  This       */
/*  function is only called once after the V5 has been powered on and        */
/*  not every time that the robot is disabled.                               */
/*---------------------------------------------------------------------------*/

void pre_auton(void) {
  // Initializing Robot Configuration. DO NOT REMOVE!
  
  vexcodeInit();
  Arm.setPosition(0,degrees);
  inertialSensor.calibrate();
  while(inertialSensor.isCalibrating()==true){
    wait(15,msec);
  }
  claw.setPosition(0,degrees);
  Controller.rumble("..-");
  selectAuton();
  claw.spin(forward,50,percent);
  // All activities that occur before the competition starts
  // Example: clearing encoders, setting servo positions, ...
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              User Control Task                            */
/*                                                                           */
/*  This task is used to control your robot during the user control phase of */
/*  a VEX Competition.                                                       */
/*                                                                           */
/*  You must modify the code to add your own robot specific commands here.   */
/*---------------------------------------------------------------------------*/

void usercontrol(void) {
  task drivetrainControl(drivetrainTask, &Competition);
  task armControl(armTask, &Competition);
  task clawControl(clawTask, &Competition);
  task toggleControl(toggleTask, &Competition);

  while (Competition.isEnabled() && Competition.isDriverControl()) {
    wait(20, msec);
  }
  drivetrainControl.stop();
  armControl.stop();
  clawControl.stop();
  toggleControl.stop();
}
//
// Main will set up the competition functions and callbacks.
//
int main() {
  // Child driver tasks must not survive a competition mode change.
  competition::bStopAllTasksBetweenModes = true;
  // Set up callbacks for autonomous and driver control periods.
  Competition.autonomous(autonomous);
  Competition.drivercontrol(usercontrol);

  // Run the pre-autonomous function.
  pre_auton();
  // Prevent main from exiting with an infinite loop.
  while (true) { 
    wait(100, msec);
  }
}
