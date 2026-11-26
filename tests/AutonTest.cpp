#include "vex.h"
#include "auton-selector.h"
#include "autonomos.h"
#include "PID.h"

#include <iostream>

namespace fake {
int nowMs = 0;
}

vex::controller Controller;
vex::motor claw;
vex::motor_group Arm;
vex::motor_group Drive;
vex::competition Competition;

namespace {
struct movement {
  bool turn;
  double target;
  double integral;
  double tolerance;
  double timeout;
};
std::vector<movement> movements;
std::vector<vex::controller> inputs;
std::size_t inputIndex = 0;
bool selecting = false;
int opticalUpdates = 0;
int visionUpdates = 0;
int toggleCalls = 0;
bool toggleAlliance = false;
bool toggleResult = true;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void selectConfiguration(bool red, bool right) {
  redAlliance = true;
  isRight = false;
  Controller = vex::controller();
  inputs.clear();
  inputIndex = 0;
  opticalUpdates = visionUpdates = 0;
  vex::controller frame;
  frame.ButtonLeft.pressed = true;
  frame.ButtonUp.pressed = red;
  frame.ButtonDown.pressed = !red;
  frame.ButtonL1.pressed = !right;
  frame.ButtonR1.pressed = right;
  // Hold the number button for multiple updates to check debouncing.
  inputs.push_back(frame);
  inputs.push_back(frame);
  inputs.push_back(vex::controller());
  frame = vex::controller();
  frame.ButtonA.pressed = true;
  inputs.push_back(frame);
  Controller = inputs.front();
  selecting = true;
  selectAuton();
  selecting = false;
  require(autonNumber == 0, "Held button changed routine more than once");
  require(redAlliance == red && isRight == right, "Incorrect starting configuration");
  const std::string expected = std::string("Set 0 ") +
      (red ? "Red" : "Blue") + (right ? " Right" : " Left");
  require(Controller.Screen.text == expected, "Confirmation omits configuration");
  require(opticalUpdates == 3 && visionUpdates == 3, "Live displays stopped updating");
}

void checkRoute(int number, bool red, bool right) {
  autonNumber = number;
  redAlliance = red;
  isRight = right;
  movements.clear();
  Arm.commands.clear();
  claw.commands.clear();
  fake::nowMs = 0;
  const int previousToggleCalls = toggleCalls;
  autonomous();
  const double direction = right ? 1.0 : -1.0;
  const std::vector<movement> expected = {
    {false, -10, 20, 1, 0},
    {true, -60 * direction, 20, 2, 0},
    {false, 20, 20, 0.8, 0},
    {true, 82 * direction, 20, 0.5, 0},
    {false, 12, 50, 0.5, 0},
    {false, -6, 20, 0.5, 0},
    {false, 10, 60, 0.5, 0},
    {false, -20, 20, 0.5, 0},
    {true, 42.5 * direction, 20, 0.3, 0}
  };
  require(movements.size() == expected.size(), "Incorrect route movement count");
  for (std::size_t i = 0; i < expected.size(); ++i) {
    const auto& actual = movements[i];
    const auto& wanted = expected[i];
    require(actual.turn == wanted.turn && actual.target == wanted.target &&
            actual.integral == wanted.integral && actual.tolerance == wanted.tolerance &&
            actual.timeout == wanted.timeout, "Route movement or PID settings changed");
  }
  require(claw.commands.size() == 4 &&
          claw.commands[0].kind == "spinFor" && claw.commands[0].value == 800 &&
          claw.commands[0].speed == 300 &&
          claw.commands[1].kind == "stop" && claw.commands[1].value == vex::hold &&
          claw.commands[2].kind == "spinFor" && claw.commands[2].value == -600 &&
          claw.commands[2].speed == 300 &&
          claw.commands[3].kind == "spinFor" && claw.commands[3].value == 150 &&
          claw.commands[3].speed == 300, "Claw actions changed");
  require(Arm.commands.size() == 4 &&
          Arm.commands[0].kind == "spin" && Arm.commands[0].value == -90 &&
          Arm.commands[1].kind == "stop" && Arm.commands[1].value == vex::brake &&
          Arm.commands[2].kind == "spin" && Arm.commands[2].value == 85 &&
          Arm.commands[3].kind == "stop" && Arm.commands[3].value == vex::brake,
          "Arm actions changed");
  require(fake::nowMs == 1100, "Mechanism wait durations changed");
  require(toggleCalls == previousToggleCalls, "Shared route unexpectedly runs active toggle");
}
}

void vex::wait(int duration, vex::timeUnits) {
  fake::nowMs += duration;
  if (selecting) {
    require(++inputIndex < inputs.size(), "Selector did not confirm");
    Controller = inputs[inputIndex];
  }
}

void updateOpticalDisplay() { ++opticalUpdates; }
void updateAIVisionDisplay() { ++visionUpdates; }
void drivePID(double target, double integral, double tolerance) {
  movements.push_back({false, target, integral, tolerance, 0});
}
void turnPID(double target, double integral, double tolerance, double timeout) {
  movements.push_back({true, target, integral, tolerance, timeout});
}
bool activeToggle(vex::competition& competition, bool red) {
  require(&competition == &Competition, "Wrong competition passed to toggle");
  ++toggleCalls;
  toggleAlliance = red;
  return toggleResult;
}

int main() {
  try {
    require(redAlliance && !isRight, "Default configuration must be Red Left");
    for (bool red : {false, true}) {
      for (bool right : {false, true}) {
        selectConfiguration(red, right);
        checkRoute(0, red, right);
        checkRoute(2, red, right);
        autonNumber = 6;
        Drive.commands.clear();
        autonomous();
        require(toggleAlliance == red, "Active toggle ignores alliance selection");
        require(Drive.commands.size() == 1 &&
                Drive.commands[0].kind == "stop" &&
                Drive.commands[0].value == vex::brake, "Routine 6 no longer stops drive");
      }
    }
    // Both wing controls must work when changing an existing selection.
    inputs = {vex::controller(), vex::controller()};
    inputs[0].ButtonL1.pressed = true;
    inputs[1].ButtonA.pressed = true;
    Controller = inputs[0];
    inputIndex = 0;
    selecting = true;
    selectAuton();
    selecting = false;
    require(!isRight && autonNumber == 1, "Cannot switch back to Left wing");
    movements.clear();
    autonomous();
    require(movements.size() == 1 && movements[0].turn &&
            movements[0].target == -90 && movements[0].integral == 2 &&
            movements[0].tolerance == 0.5 && movements[0].timeout == 3000,
            "Routine 1 changed");
    toggleResult = false;
    require(!activeToggle(), "Toggle failure was not propagated");
    std::cout << "PASS autonomous selection, four configurations, mirrored routes, and aliases\n";
  } catch (const std::exception& error) {
    std::cerr << "FAIL autonomous: " << error.what() << '\n';
    return 1;
  }
}
