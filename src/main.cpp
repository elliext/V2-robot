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
#include "AIVision.h"
#include "AssistDrive.h"
#if defined(AI_VISION_TEST) || defined(AUTON_SELECTION_TEST) || defined(AIM_TEST)
#error Hardware-test startup modes were removed. Build without hardware-test defines.
#endif
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
/*  function is only called once each time the program is launched and      */
/*  not every time that the robot is disabled.                               */
/*---------------------------------------------------------------------------*/

void pre_auton(void) {
  // Initializing Robot Configuration. DO NOT REMOVE!
  
  vexcodeInit();
  selectAuton();
  Arm.setPosition(0,degrees);
  if(inertialSensor.installed()){
    inertialSensor.calibrate();
    timer calibrationTimer;
    while(inertialSensor.isCalibrating() && calibrationTimer.time(msec)<5000){
      wait(15,msec);
    }
    if(inertialSensor.isCalibrating()){
      printf("Pre-auton: inertial calibration timed out after 5 seconds; autonomous turns remain unavailable while calibrating.\n");
      Controller.rumble("---");
    }
  }
  else{
    printf("Pre-auton: inertial sensor missing on port 1; skipping calibration. Autonomous turns require this sensor.\n");
  }
  claw.setPosition(0,degrees);
  Controller.rumble("..-");
  claw.spin(forward,50,percent);
  enableToggleLights();
  updateAIVisionDisplay();
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
  motorizedToggleing(true);
}
//
// Main will set up the competition functions and callbacks.
//
int main() {
  // Child driver tasks must not survive a competition mode change.
  competition::bStopAllTasksBetweenModes = true;
  // Confirm wing and alliance before any competition callback can move the robot.
  pre_auton();
  // Set up callbacks for autonomous and driver control periods.
  Competition.autonomous(autonomous);
  Competition.drivercontrol(usercontrol);
  // Prevent main from exiting with an infinite loop.
  while (true) { 
    updateOpticalDisplay();
    updateAIVisionDisplay();
    wait(100, msec);
  }
}
