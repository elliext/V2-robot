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
  updateAIVisionDisplay();
  Arm.setPosition(0,degrees);
  if(inertialSensor.installed()){
    inertialSensor.calibrate();
    while(inertialSensor.isCalibrating()==true){
      wait(15,msec);
    }
  }
  else{
    printf("Pre-auton: inertial sensor missing on port 1; skipping calibration. Autonomous turns require this sensor.\n");
  }
  claw.setPosition(0,degrees);
  Controller.rumble("..-");
  selectAuton();
  claw.spin(forward,50,percent);
  enableToggleLights();
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
#if (defined(AI_VISION_TEST) + defined(AUTON_SELECTION_TEST) + defined(AIM_TEST)) > 1
#error Select only one hardware test mode.
#elif defined(AIM_TEST)
  vexcodeInit();
  if(Competition.isFieldControl() || Competition.isCompetitionSwitch()){
    printf("Aim test: disconnect field control or the competition switch before testing.\n");
    Brain.Screen.printAt(10, 30, true, "Aim test: disconnect field control.");
    return 1;
  }
  // Driver mode unmasks the controller; no motor callbacks are registered, and
  // assistDrive() only displays its output in this build.
  Competition.test_driver();
  printf("Aim test: motor-free; hold Up (cup) or Down (goal).\n");
  Brain.Screen.render();
  assistReset();
  while(true){
    assistDrive();
    wait(20, msec);
  }
#elif defined(AI_VISION_TEST)
  vexcodeInit();
  if(Competition.isFieldControl() || Competition.isCompetitionSwitch()){
    printf("Vision test: disconnect field control or the competition switch before testing.\n");
    Brain.Screen.printAt(10, 30, true, "Vision test: disconnect field control.");
    return 1;
  }
  printf("Vision test: motor-free; no controller selection or competition callbacks.\n");
  bool renderFault = false;
  if (!Brain.Screen.render()) {
    printf("Vision test: screen buffering initialization failed.\n");
    renderFault = true;
  }
  while(true){
    updateAIVisionDisplay();
    const bool rendered = Brain.Screen.render();
    if (!rendered && !renderFault) {
      printf("Vision test: screen frame presentation failed.\n");
    } else if (rendered && renderFault) {
      printf("Vision test: screen frame presentation recovered.\n");
    }
    renderFault = !rendered;
    wait(20, msec);
  }
#elif defined(AUTON_SELECTION_TEST)
  vexcodeInit();
  if(Competition.isFieldControl() || Competition.isCompetitionSwitch()){
    printf("Selection test: disconnect field control or the competition switch before testing.\n");
    Brain.Screen.printAt(10, 30, true, "Selection test: disconnect field control.");
    return 1;
  }
  // Autonomous mode masks controller input; no motor callbacks are registered here.
  Competition.test_driver();
  printf("Selection test: motor-free; driver input enabled.\n");
  selectAuton();
  while(true){
    updateOpticalDisplay();
    updateAIVisionDisplay();
    wait(100, msec);
  }
#else
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
#endif
}
