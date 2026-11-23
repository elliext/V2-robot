#include "vex.h"

using namespace vex;

// A global instance of brain used for printing to the V5 brain screen
brain Brain;
//12, 13, 11, 20, 19, 18
motor LeftFront = motor(PORT14, ratio6_1, true);
motor LeftBack = motor(PORT20, ratio6_1, true);

motor RightFront = motor(PORT4, ratio6_1, false);
motor RightBack = motor(PORT10, ratio6_1, true);

motor_group LeftDrive = motor_group(LeftFront, LeftBack);
motor_group RightDrive = motor_group(RightFront, RightBack);
motor_group Drive = motor_group(LeftFront, LeftBack, RightFront, RightBack);

motor ArmLeft = motor (PORT12, ratio36_1, false);
motor ArmRight = motor (PORT2, ratio36_1, true);
motor_group Arm = motor_group(ArmLeft, ArmRight);

motor claw = motor (PORT11, ratio18_1,true);

motor toggle = motor (PORT13, ratio18_1,false);

inertial inertialSensor = inertial(PORT16);
controller Controller = controller(primary);

/**
 * Used to initialize code/tasks/devices added using tools in VEXcode Pro.
 *
 * This should be called at the start of your int main function.
 */
void vexcodeInit(void) {
  // Nothing to initialize
}