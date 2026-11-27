#include "vex.h"
#include "match-setup.h"
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
  redAlliance = !red;
  isRight = !right;
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
  // Holding selection buttons must not trigger redundant redraws.
  inputs.push_back(frame);
  inputs.push_back(frame);
  inputs.push_back(vex::controller());
  frame = vex::controller();
  frame.ButtonA.pressed = true;
  inputs.push_back(frame);
  Controller = inputs.front();
  selecting = true;
  selectMatchSetup();
  selecting = false;
  require(redAlliance == red && isRight == right, "Incorrect starting configuration");
  std::string expected = std::string("Set ") +
      (red ? "Red" : "Blue") + (right ? " Right" : " Left");
  expected.resize(20, ' ');
  require(Controller.Screen.text == expected, "Confirmation omits configuration");
  require(Controller.Screen.clearCalls == 0, "Selection redraw clears and flickers");
  require(Controller.Screen.printCalls == 3, "Selection rendered repeatedly within an update");
  require(Controller.rumbleCalls == 1, "Confirmation should rumble once");
  require(opticalUpdates == 3 && visionUpdates == 3, "Live displays stopped updating");
}

void checkRoute(bool red, bool right) {
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
    if (inputIndex == 0) {
      require(Controller.Screen.text.find("Setup ") == 0, "Match setup prompt not displayed");
    }
    require(++inputIndex < inputs.size(), "Match setup did not confirm");
    const auto& frame = inputs[inputIndex];
    Controller.ButtonA = frame.ButtonA;
    Controller.ButtonLeft = frame.ButtonLeft;
    Controller.ButtonRight = frame.ButtonRight;
    Controller.ButtonUp = frame.ButtonUp;
    Controller.ButtonDown = frame.ButtonDown;
    Controller.ButtonL1 = frame.ButtonL1;
    Controller.ButtonR1 = frame.ButtonR1;
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
        checkRoute(red, right);
        require(activeToggle(), "Active toggle success was not propagated");
        require(toggleAlliance == red, "Active toggle ignores alliance selection");
      }
    }
    // Both wing controls must work when changing an existing selection.
    inputs = {vex::controller(), vex::controller()};
    inputs[0].ButtonL1.pressed = true;
    inputs[1].ButtonA.pressed = true;
    Controller = inputs[0];
    inputIndex = 0;
    selecting = true;
    selectMatchSetup();
    selecting = false;
    require(!isRight, "Cannot switch back to Left wing");
    toggleResult = false;
    require(!activeToggle(), "Toggle failure was not propagated");
    redAlliance = true;
    isRight = false;
    inputs.assign(5, vex::controller());
    inputs[0].ButtonA.pressed = true;
    inputs[1].ButtonA.pressed = true;
    inputs[2].ButtonDown.pressed = true;
    inputs[2].ButtonR1.pressed = true;
    inputs[4].ButtonA.pressed = true;
    Controller = inputs[0];
    inputIndex = 0;
    selecting = true;
    selectMatchSetup();
    selecting = false;
    require(inputIndex == 4 && !redAlliance && isRight,
            "A held at launch confirmed before a new press");
    require(Controller.Screen.text == "Set Blue Right      ", "Held-A confirmation incorrect");

    inputs.assign(5, vex::controller());
    inputs[0].ButtonLeft.pressed = inputs[1].ButtonLeft.pressed = true;
    inputs[2].ButtonRight.pressed = inputs[3].ButtonRight.pressed = true;
    inputs[4].ButtonA.pressed = true;
    Controller = inputs[0];
    inputIndex = 0;
    selecting = true;
    selectMatchSetup();
    selecting = false;
    require(!redAlliance && isRight, "Unused directional arrows changed configuration");
    require(Controller.Screen.printCalls == 2, "Unused arrows caused selection redraws");
    require(Controller.Screen.text == "Set Blue Right      ", "Routine number remains in display");
    std::cout << "PASS match setup, mirrored autonomous, and unused routine arrows\n";
  } catch (const std::exception& error) {
    std::cerr << "FAIL autonomous: " << error.what() << '\n';
    return 1;
  }
}
