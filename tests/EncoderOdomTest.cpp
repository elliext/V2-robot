#include "vex.h"
#include "EncoderOdom.h"

#include <cmath>
#include <limits>

vex::motor LeftFront;
vex::motor RightFront;
vex::inertial inertialSensor;

namespace {
const double kPi = 3.141592653589793;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void near(double actual, double expected) {
  require(std::fabs(actual - expected) < 1e-8, "Unexpected odometry value");
}

void travel(double leftInches, double rightInches, double heading) {
  const double degreesPerInch = -360.0 * (5.0 / 3.0) / (kPi * 3.25);
  LeftFront.positionDegrees += leftInches * degreesPerInch;
  RightFront.positionDegrees += rightInches * degreesPerInch;
  inertialSensor.headingDegrees = heading;
}
}

int main() {
  EncoderOdom odom;
  require(!odom.update(), "Update must require initialization");
  // Reset captures existing readings without zeroing hardware.
  LeftFront.positionDegrees = 100;
  RightFront.positionDegrees = 200;
  inertialSensor.headingDegrees = 27;
  require(odom.reset(), "Reset failed");
  near(LeftFront.positionDegrees, 100);
  near(inertialSensor.headingDegrees, 27);
  travel(12, 12, 27);
  require(odom.update(), "Forward update failed");
  near(odom.getPose().x, 0);
  near(odom.getPose().y, 12);
  travel(-5, -5, 27);
  require(odom.update(), "Reverse update failed");
  near(odom.getPose().y, 7);
  require(odom.update(), "Stationary update failed");
  near(odom.getPose().y, 7);

  require(odom.reset(3, 4, 90), "Custom reset failed");
  travel(10, 10, 27);
  require(odom.update(), "Rotated forward update failed");
  near(odom.getPose().x, 13);
  near(odom.getPose().y, 4);
  travel(5, -5, 117);
  require(odom.update(), "In-place turn failed");
  near(odom.getPose().x, 13);
  near(odom.getPose().y, 4);
  near(odom.getPose().headingDegrees, 180);

  inertialSensor.headingDegrees = 0;
  require(odom.reset(), "Arc reset failed");
  travel(8, 12, 90);
  require(odom.update(), "Arc update failed");
  near(odom.getPose().x, 20 / kPi);
  near(odom.getPose().y, 20 / kPi);

  inertialSensor.headingDegrees = 359;
  require(odom.reset(0, 0, 359), "Wrap reset failed");
  travel(0, 0, 1);
  require(odom.update(), "Clockwise wrap failed");
  near(odom.getPose().headingDegrees, 361);
  travel(0, 0, 359);
  require(odom.update(), "Counterclockwise wrap failed");
  near(odom.getPose().headingDegrees, 359);

  require(!odom.reset(std::numeric_limits<double>::infinity()),
          "Invalid starting pose accepted");
  near(odom.getPose().headingDegrees, 359);
  LeftFront.connected = false;
  require(!odom.update(), "Missing motor accepted");
  LeftFront.connected = true;
  require(!odom.update(), "Sensor recovery must require reset");
  inertialSensor.connected = false;
  require(!odom.reset(), "Missing inertial accepted");
  inertialSensor.connected = true;
  inertialSensor.calibrating = true;
  require(!odom.reset(), "Calibrating inertial accepted");
  inertialSensor.calibrating = false;
  require(odom.reset(), "Recovery reset failed");
  RightFront.positionDegrees = std::numeric_limits<double>::quiet_NaN();
  require(!odom.update(), "Invalid motor reading accepted");
  RightFront.positionDegrees = 0;
  require(odom.reset(), "Second recovery reset failed");
  inertialSensor.headingDegrees = std::numeric_limits<double>::quiet_NaN();
  require(!odom.update(), "Invalid heading accepted");
  require(LeftFront.commands.empty() && RightFront.commands.empty(),
          "Odometry must not command motors");
  std::puts("Encoder odometry tests passed.");
}
