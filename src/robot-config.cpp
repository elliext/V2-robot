#include "vex.h"

using namespace vex;

// A global instance of brain used for printing to the V5 brain screen
brain Brain;
//12, 13, 11, 20, 19, 18
motor LeftFront = motor(PORT14, ratio6_1, true);
motor LeftBack = motor(PORT20, ratio6_1, false);

motor RightFront = motor(PORT4, ratio6_1, false);
motor RightBack = motor(PORT10, ratio6_1, true);

motor_group LeftDrive = motor_group(LeftFront, LeftBack);
motor_group RightDrive = motor_group(RightFront, RightBack);
motor_group Drive = motor_group(LeftFront, LeftBack, RightFront, RightBack);

motor ArmLeft = motor (PORT12, ratio36_1, true);
motor ArmRight = motor (PORT2, ratio36_1, false);
motor_group Arm = motor_group(ArmLeft, ArmRight);

motor claw = motor (PORT11, ratio18_1,true);

motor toggleLeft = motor (PORT13, ratio18_1,false);
motor toggleRight = motor (PORT3, ratio18_1,true);

inertial inertialSensor = inertial(PORT1);

controller Controller = controller(primary);

optical toggleOptical1 = optical(PORT5);
optical toggleOptical2 = optical(PORT15);



/**
 * Used to initialize code/tasks/devices added using tools in VEXcode Pro.
 *
 * This should be called at the start of your int main function.
 */
void vexcodeInit(void) {
  // Nothing to initialize
}