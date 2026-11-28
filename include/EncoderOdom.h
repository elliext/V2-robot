#pragma once

struct EncoderPose {
  double x;
  double y;
  double headingDegrees;
};

class EncoderOdom {
public:
  bool reset(double x = 0, double y = 0, double headingDegrees = 0);
  bool update();
  EncoderPose getPose() const;

private:
  bool readSensors(double& left, double& right, double& heading);
  EncoderPose pose = {0, 0, 0};
  double previousLeft = 0;
  double previousRight = 0;
  double previousHeading = 0;
  bool initialized = false;
};
