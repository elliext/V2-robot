#include "PID.h"

#include <limits>

vex::motor LeftBack;
vex::motor_group LeftDrive;
vex::motor_group RightDrive;
vex::motor_group Drive;
vex::motor_group Arm;
vex::inertial inertialSensor;
vex::controller Controller;

namespace fake {
int nowMs = 0;
std::vector<double> rotations;
std::size_t sample = 0;
int waits = 0;
bool disconnectOnWait = false;
}

namespace vex {
void wait(int duration, timeUnits) {
  if(++fake::waits>1000){
    throw std::runtime_error("Turn PID did not terminate");
  }
  fake::nowMs += duration;
  if(duration==50){
    return;
  }
  if(fake::disconnectOnWait){
    inertialSensor.connected = false;
  }
  if(fake::sample<fake::rotations.size()){
    inertialSensor.rotationDegrees = fake::rotations[fake::sample++];
  }
}
}

namespace {
void require(bool condition, const char* message) {
  if(!condition){
    throw std::runtime_error(message);
  }
}

void prepare(std::vector<double> rotations = {}) {
  fake::nowMs = 0;
  fake::rotations = rotations;
  fake::sample = 0;
  fake::waits = 0;
  fake::disconnectOnWait = false;
  LeftDrive = vex::motor_group();
  RightDrive = vex::motor_group();
  Drive = vex::motor_group();
  inertialSensor = vex::inertial();
  Controller = vex::controller();
}

void checkBraked() {
  require(!Drive.commands.empty() && Drive.commands.back().kind=="stop" &&
          Drive.commands.back().value==vex::brake, "Turn must end with braking");
}
}

int main() {
  prepare({90});
  turnPID(90, 20, 0.5, 4500);
  require(Controller.rumbleCalls==0, "Settled turn should succeed");
  require(fake::nowMs>=230, "Turn exited before settling");
  checkBraked();
  require(LeftDrive.commands.front().value<0 &&
          RightDrive.commands.front().value>0, "Positive turn direction changed");

  prepare({90, 92, 90});
  turnPID(90, 20, 0.5, 4500);
  require(fake::sample==3 && fake::nowMs>=260,
          "Overshoot must restart the settling interval");
  require(LeftDrive.commands.size()>=2, "Overshoot correction did not resume");
  require(Controller.rumbleCalls==0, "Recovered overshoot should succeed");

  prepare({-90});
  turnPID(-90, 20, 0.5, 4500);
  require(Controller.rumbleCalls==0, "Negative turn failed");
  require(LeftDrive.commands.front().value>0 &&
          RightDrive.commands.front().value<0, "Negative turn direction changed");

  prepare();
  turnPID(90, 20, 0.5, 4500);
  require(fake::nowMs==4550 && Controller.rumbleCalls==1,
          "Timeout must include settling and retain the final 50 ms delay");
  checkBraked();

  prepare({90});
  turnPID(90, 20, 0.5, 100);
  require(fake::nowMs==150 && Controller.rumbleCalls==1,
          "Angle inside tolerance without enough settling time is not success");

  prepare({89.6, 90});
  turnPID(90, 20, 0.5, 180);
  require(Controller.rumbleCalls==1,
          "High angular speed inside tolerance must postpone settling");

  prepare({180});
  turnPID(180, 500, 0.5, 4500);
  for(const auto& command : LeftDrive.commands){
    require(std::isfinite(command.value) && std::fabs(command.value)<=100,
            "Motor commands must be finite and clamped to 100 percent");
  }

  prepare({90});
  turnPID(90, 20, 0.5);
  require(Controller.rumbleCalls==0, "Default no-timeout turn failed");

  prepare();
  fake::disconnectOnWait = true;
  turnPID(90, 20, 0.5, 4500);
  require(Controller.rumbleCalls==1 && fake::nowMs==65,
          "Disconnected inertial must abort promptly");
  checkBraked();

  prepare({std::numeric_limits<double>::quiet_NaN()});
  turnPID(90, 20, 0.5, 4500);
  require(Controller.rumbleCalls==1, "Non-finite sensor reading must abort");
  checkBraked();

  prepare();
  turnPID(std::numeric_limits<double>::infinity(), 20, 0.5, 4500);
  turnPID(90, -1, 0.5, 4500);
  turnPID(90, 20, 0, 4500);
  turnPID(90, 20, 0.5, -1);
  require(Controller.rumbleCalls==4 && LeftDrive.commands.empty(),
          "Invalid inputs must not start a turn");

  prepare();
  inertialSensor.calibrating = true;
  turnPID(90, 20, 0.5, 4500);
  require(Controller.rumbleCalls==1 && LeftDrive.commands.empty(),
          "Calibration must prevent motor motion");
  std::puts("Turn PID tests passed.");
}
