using namespace vex;

extern brain Brain;

extern motor LeftFront;
extern motor LeftMiddle;
extern motor LeftBack;
extern motor RightFront;
extern motor RightMiddle;
extern motor RightBack;
extern motor_group LeftDrive;
extern motor_group RightDrive;
extern motor_group Drive;

extern motor claw;

extern motor ArmLeft;
extern motor ArmRight;
extern motor_group Arm;

extern motor toggleLeft;
extern motor toggleRight;

extern optical toggleOptical1;
extern optical toggleOptical2;

extern inertial inertialSensor;
//Yellow = 31 - 70
//Red = 0 - 31, 330 - 360
//Blue = 100 - 330

extern controller Controller;

/**
 * Used to initialize code/tasks/devices added using tools in VEXcode Pro.
 *
 * This should be called at the start of your int main function.
 */
void vexcodeInit(void);
