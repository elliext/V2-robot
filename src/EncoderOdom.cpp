#include "vex.h"
#include "EncoderOdom.h"
#include "robot-config.h"

#include <cmath>

namespace {
const double kPi = 3.141592653589793;
// Match the current drivePID conversion; verify against the physical drivetrain.
const double kWheelDiameterInches = 3.25;
const double kMotorRevolutionsPerWheelRevolution = 5.0 / 3.0;
const double kLeftForwardSign = -1;
const double kRightForwardSign = -1;

double motorDegreesToInches(double motorDegrees) {
  return motorDegrees / 360.0 * kPi * kWheelDiameterInches /
      kMotorRevolutionsPerWheelRevolution;
}
}

bool EncoderOdom::readSensors(double& left, double& right, double& heading) {
  if (!LeftFront.installed() || !RightFront.installed() ||
      !inertialSensor.installed() || inertialSensor.isCalibrating()) {
    printf("Encoder odom: front motors or inertial sensor unavailable; reset required.\n");
    initialized = false;
    return false;
  }
  // PID resets LeftBack.position() and inertial.rotation(), not these readings.
  left = kLeftForwardSign * motorDegreesToInches(LeftFront.position(degrees));
  right = kRightForwardSign * motorDegreesToInches(RightFront.position(degrees));
  heading = inertialSensor.heading(degrees);
  if (!std::isfinite(left) || !std::isfinite(right) ||
      !std::isfinite(heading)) {
    printf("Encoder odom: non-finite sensor reading; reset required.\n");
    initialized = false;
    return false;
  }
  return true;
}

bool EncoderOdom::reset(double x, double y, double headingDegrees) {
  if (!std::isfinite(x) || !std::isfinite(y) ||
      !std::isfinite(headingDegrees)) {
    printf("Encoder odom: starting pose must be finite.\n");
    return false;
  }
  double left, right, heading;
  if (!readSensors(left, right, heading)) {
    return false;
  }
  pose = {x, y, headingDegrees};
  previousLeft = left;
  previousRight = right;
  previousHeading = heading;
  initialized = true;
  return true;
}

bool EncoderOdom::update() {
  if (!initialized) {
    printf("Encoder odom: call reset after calibration before updating.\n");
    return false;
  }
  double left, right, heading;
  if (!readSensors(left, right, heading)) {
    return false;
  }
  const double distance =
      ((left - previousLeft) + (right - previousRight)) / 2.0;
  // Heading wraps at 360; less than 180 degrees may pass between updates.
  const double headingChange = std::remainder(heading - previousHeading, 360.0);
  const double angleChange = headingChange * kPi / 180.0;
  const double middleAngle =
      (pose.headingDegrees + headingChange / 2.0) * kPi / 180.0;
  const double arcScale = std::fabs(angleChange) < 1e-6
      ? 1.0 : 2.0 * std::sin(angleChange / 2.0) / angleChange;
  pose.x += distance * arcScale * std::sin(middleAngle);
  pose.y += distance * arcScale * std::cos(middleAngle);
  pose.headingDegrees += headingChange;
  previousLeft = left;
  previousRight = right;
  previousHeading = heading;
  return true;
}

EncoderPose EncoderOdom::getPose() const {
  return pose;
}
