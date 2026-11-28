#include "vex.h"
#include "auton-selector.h"
#include "autonomos.h"
#include "PID.h"

#include <iostream>

namespace fake {
int nowMs = 0;
}

vex::controller Controller;
vex::brain Brain;
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
std::size_t movementsAtToggle = 0;
std::vector<vex::brain::screen> touchInputs;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void selectConfiguration(bool red, bool right) {
  redAlliance = !red;
  Controller = vex::controller();
  inputs.clear();
  inputIndex = 0;
  opticalUpdates = visionUpdates = 0;
  vex::controller frame;
  frame.ButtonLeft.pressed = !right;
  frame.ButtonRight.pressed = right;
  frame.ButtonUp.pressed = red;
  frame.ButtonDown.pressed = !red;
  frame.ButtonL1.pressed = right;
  frame.ButtonR1.pressed = !right;
  // Holding selection buttons must not trigger redundant redraws.
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
  require(redAlliance == red, "Incorrect alliance selection");
  require(selectedWing == (right ? AutonWing::Right : AutonWing::Left),
          "Incorrect wing selection");
  std::string expected = std::string(right ? "Set Right Wing " : "Set Left Wing ") +
      (red ? "Red" : "Blue");
  expected.resize(20, ' ');
  require(Controller.Screen.text == expected, "Confirmation omits configuration");
  require(Controller.Screen.clearCalls == 0, "Selection redraw clears and flickers");
  require(Controller.Screen.printCalls == 3, "Selection rendered repeatedly within an update");
  require(Controller.rumbleCalls == 1, "Confirmation should rumble once");
  require(opticalUpdates == 3 && visionUpdates == 0,
          "Optical readings should remain live without overwriting the Brain menu");
  require(!Brain.Screen.lines.empty() &&
          Brain.Screen.lines.back().text.find("Confirmed: ") == 0,
          "Brain confirmation is missing");
}

void checkScoring(bool right) {
  movements.clear();
  Arm.commands.clear();
  claw.commands.clear();
  fake::nowMs = 0;
  const int previousToggleCalls = toggleCalls;
  if (right) {
    scoreRightAllianceGoal();
  } else {
    scoreLeftAllianceGoal();
  }
  require(movements.size() == 1 &&
          !movements[0].turn && movements[0].target == -10 &&
          movements[0].integral == 20 && movements[0].tolerance == 1 &&
          movements[0].timeout == 0, "Scoring drive/PID settings changed");
  require(claw.commands.size() == 3 &&
          claw.commands[0].kind == "spinFor" && claw.commands[0].value == 800 &&
          claw.commands[0].speed == 300 &&
          claw.commands[1].kind == "stop" && claw.commands[1].value == vex::hold &&
          claw.commands[2].kind == "spinFor" && claw.commands[2].value == -600 &&
          claw.commands[2].speed == 300, "Scoring claw actions changed");
  require(Arm.commands.size() == 3 &&
          Arm.commands[0].kind == "spin" && Arm.commands[0].value == -90 &&
          Arm.commands[1].kind == "stop" && Arm.commands[1].value == vex::brake &&
          Arm.commands[2].kind == "spin" && Arm.commands[2].value == 85,
          "Scoring arm actions changed");
  require(fake::nowMs == 1100, "Scoring wait durations changed");
  require(toggleCalls == previousToggleCalls, "Scoring unexpectedly runs active toggle");
}

void checkRoute(bool red, bool right, bool direct = false) {
  selectedWing = right ? AutonWing::Right : AutonWing::Left;
  redAlliance = red;
  movements.clear();
  Arm.commands.clear();
  claw.commands.clear();
  fake::nowMs = 0;
  const int previousToggleCalls = toggleCalls;
  if(direct){
    if(right){
      rightWing();
    }
    else{
      leftWing();
    }
  }
  else{
    autonomous();
  }
  const double direction = right ? 1.0 : -1.0;
  const std::vector<movement> expected = {
    {true, 75 * direction, 20, 0.25, 0},
    {false, 15, 20, 0.8, 0}
  };
  require(movements.size() == expected.size(), "Incorrect route movement count");
  for (std::size_t i = 0; i < expected.size(); ++i) {
    const auto& actual = movements[i];
    const auto& wanted = expected[i];
    require(actual.turn == wanted.turn && actual.target == wanted.target &&
            actual.integral == wanted.integral && actual.tolerance == wanted.tolerance &&
            actual.timeout == wanted.timeout, "Route movement or PID settings changed");
  }
  require(claw.commands.empty(), "Wing route unexpectedly commands the claw");
  require(Arm.commands.size() == 1 &&
          Arm.commands[0].kind == "spinForRotation" && Arm.commands[0].value == 200 &&
          Arm.commands[0].speed == 300 && !Arm.lastSpinWaitForCompletion,
          "Arm actions changed");
  require(fake::nowMs == 0, "Wing route added a blocking mechanism wait");
  require(toggleCalls == previousToggleCalls+1 && movementsAtToggle == 0,
          "Wing route must run the active toggle before moving");
  require(toggleAlliance == red, "Wing route ignores selected alliance");
}
}

void vex::wait(int duration, vex::timeUnits) {
  fake::nowMs += duration;
  if (selecting) {
    if (inputIndex == 0) {
      require(Controller.Screen.text.find("Right Wing ") == 0 ||
              Controller.Screen.text.find("Left Wing ") == 0,
              "Wing selector prompt not displayed");
    }
    require(++inputIndex < inputs.size(), "Auton selector did not confirm");
    const auto& frame = inputs[inputIndex];
    Controller.ButtonA = frame.ButtonA;
    Controller.ButtonLeft = frame.ButtonLeft;
    Controller.ButtonRight = frame.ButtonRight;
    Controller.ButtonUp = frame.ButtonUp;
    Controller.ButtonDown = frame.ButtonDown;
    Controller.ButtonL1 = frame.ButtonL1;
    Controller.ButtonR1 = frame.ButtonR1;
    if(!touchInputs.empty()){
      const auto& touch = touchInputs.at(inputIndex);
      Brain.Screen.touchDown = touch.touchDown;
      Brain.Screen.touchX = touch.touchX;
      Brain.Screen.touchY = touch.touchY;
    }
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
  movementsAtToggle = movements.size();
  toggleAlliance = red;
  return toggleResult;
}

int main() {
  try {
    require(redAlliance && selectedWing == AutonWing::Right,
            "Default configuration must be Red Right Wing");
    checkScoring(false);
    checkScoring(true);
    for (bool red : {false, true}) {
      for (bool right : {false, true}) {
        selectConfiguration(red, right);
        checkRoute(red, right);
        checkRoute(red, right, true);
        const int previousToggleCalls = toggleCalls;
        require(activeToggle(), "Toggle success was not propagated");
        require(toggleCalls == previousToggleCalls + 1, "Active toggle wrapper did not run");
        require(toggleAlliance == red, "Active toggle ignores alliance selection");
      }
    }
    inputs = {vex::controller(), vex::controller()};
    inputs[0].ButtonL1.pressed = true;
    inputs[1].ButtonA.pressed = true;
    Controller = inputs[0];
    inputIndex = 0;
    selecting = true;
    selectAuton();
    selecting = false;
    require(selectedWing == AutonWing::Right, "L1 changed wing during selection");
    toggleResult = false;
    require(!activeToggle(), "Toggle failure was not propagated");
    toggleResult = true;
    redAlliance = true;
    inputs.assign(5, vex::controller());
    inputs[0].ButtonA.pressed = true;
    inputs[1].ButtonA.pressed = true;
    inputs[2].ButtonDown.pressed = true;
    inputs[2].ButtonR1.pressed = true;
    inputs[4].ButtonA.pressed = true;
    Controller = inputs[0];
    inputIndex = 0;
    selecting = true;
    selectAuton();
    selecting = false;
    require(inputIndex == 4 && !redAlliance && selectedWing == AutonWing::Right,
            "A held at launch confirmed before a new press");
    require(Controller.Screen.text == "Set Right Wing Blue ", "Held-A confirmation incorrect");

    inputs.assign(5, vex::controller());
    inputs[0].ButtonLeft.pressed = inputs[1].ButtonLeft.pressed = true;
    inputs[2].ButtonLeft.pressed = inputs[2].ButtonRight.pressed = true;
    inputs[3].ButtonRight.pressed = true;
    inputs[4].ButtonA.pressed = true;
    Controller = inputs[0];
    inputIndex = 0;
    selecting = true;
    selectAuton();
    selecting = false;
    require(!redAlliance, "Wing arrows changed alliance");
    require(selectedWing == AutonWing::Right && Controller.Screen.printCalls == 4,
            "Wing selection redrew held buttons or changed on simultaneous arrows");
    require(Controller.Screen.text == "Set Right Wing Blue ", "Right Wing label not displayed");

    inputs.assign(4, vex::controller());
    inputs[0].ButtonLeft.pressed = true;
    inputs[2].ButtonLeft.pressed = true;
    inputs[3].ButtonA.pressed = true;
    Controller = inputs[0];
    inputIndex = 0;
    selecting = true;
    selectAuton();
    selecting = false;
    require(selectedWing == AutonWing::Left && Controller.Screen.text == "Set Left Wing Blue  ",
            "Left Wing selection or confirmation incorrect");
    checkRoute(false, false);
    // Both touchscreen choices must work with no controller buttons pressed.
    for(bool right : {false, true}){
      for(bool red : {false, true}){
        redAlliance = !red;
        Brain.Screen = vex::brain::screen();
        inputs.assign(7, vex::controller());
        touchInputs.assign(7, vex::brain::screen());
        touchInputs[1].touchDown = touchInputs[2].touchDown = true;
        touchInputs[1].touchX = touchInputs[2].touchX = right ? 300 : 50;
        touchInputs[1].touchY = touchInputs[2].touchY = 70;
        touchInputs[4].touchDown = true;
        touchInputs[4].touchX = red ? 50 : 300;
        touchInputs[4].touchY = 140;
        touchInputs[6].touchDown = true;
        touchInputs[6].touchX = 100;
        touchInputs[6].touchY = 220;
        Controller = inputs[0];
        inputIndex = 0;
        selecting = true;
        selectAuton();
        selecting = false;
        require(selectedWing == (right ? AutonWing::Right : AutonWing::Left) &&
                redAlliance == red, "Touchscreen selection failed");
        require(inputIndex == 6, "Touchscreen confirmed before Confirm was tapped");
        checkRoute(red, right);
        touchInputs.clear();
        Brain.Screen.touchDown = false;
      }
    }
    // A held Confirm touch at startup must be released and tapped again.
    inputs.assign(4, vex::controller());
    touchInputs.assign(4, vex::brain::screen());
    touchInputs[0].touchDown = touchInputs[1].touchDown = touchInputs[3].touchDown = true;
    for(auto& touch : touchInputs){
      touch.touchX = 100;
      touch.touchY = 220;
    }
    Controller = inputs[0];
    Brain.Screen.touchDown = true;
    Brain.Screen.touchX = 100;
    Brain.Screen.touchY = 220;
    inputIndex = 0;
    selecting = true;
    selectAuton();
    selecting = false;
    require(inputIndex == 3, "Held Confirm touch confirmed at startup");
    touchInputs.clear();
    Brain.Screen.touchDown = false;
    toggleResult = false;
    for(bool right : {false, true}){
      selectedWing = right ? AutonWing::Right : AutonWing::Left;
      movements.clear();
      Arm.commands.clear();
      autonomous();
      require(movements.empty() && Arm.commands.empty(),
              "Wing route continued after active toggle failure");
    }
    const int previousToggleCalls = toggleCalls;
    selectedWing = static_cast<AutonWing>(2);
    Drive.commands.clear();
    autonomous();
    require(Drive.commands.size() == 1 &&
            Drive.commands[0].value == vex::brake &&
            toggleCalls == previousToggleCalls,
            "Invalid wing selection did not stop safely");
    std::cout << "PASS Right/Left Wing selector, paths, and active toggle wrapper\n";
  } catch (const std::exception& error) {
    std::cerr << "FAIL autonomous: " << error.what() << '\n';
    return 1;
  }
}
