#include "vex.h"
#include "Toggle.h"

#include <functional>
#include <iostream>

namespace fake {
int nowMs = 0;
}

vex::controller Controller;
vex::optical toggleOptical1;
vex::optical toggleOptical2;
vex::motor toggleLeft;
vex::motor toggleRight;
bool redAlliance = true;
std::function<void()> onWait;

void vex::wait(int duration, vex::timeUnits) {
  fake::nowMs += duration;
  if(onWait){
    onWait();
  }
}

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void updateAt(int time) {
  fake::nowMs = time;
  updateOpticalDisplay();
}

void requireTurning(const vex::motor& motor) {
  require(!motor.commands.empty() && motor.commands.back().kind == "spin" &&
          motor.commands.back().value == -100, "Wrong-color toggle did not resume turning");
}

void requireBraked(const vex::motor& motor) {
  require(!motor.commands.empty() && motor.commands.back().kind == "stop" &&
          motor.commands.back().value == vex::brake, "Target-color toggle did not brake");
}

void checkActiveToggle() {
  toggleOptical1.connected = toggleOptical2.connected = true;
  enableToggleLights();
  vex::competition competition;
  competition.enabled = competition.autonomous = true;
  for(bool red : {true, false}){
    const double targetHue = red ? 0 : 220;
    const double wrongHue = red ? 220 : 0;
    toggleOptical1.nearby = toggleOptical2.nearby = true;
    toggleOptical2.sampleHue = targetHue;
    toggleOptical1.sampleHue = wrongHue;
    const int startMs = fake::nowMs;
    onWait = [=] {
      const int elapsed = fake::nowMs - startMs;
      if(elapsed == 20){
        toggleOptical2.sampleHue = wrongHue;
      }
      if(elapsed == 40){
        requireTurning(toggleLeft);
      }
      if(elapsed == 60){
        toggleOptical2.sampleHue = targetHue;
      }
      if(elapsed == 180){
        requireBraked(toggleLeft);
        requireTurning(toggleRight);
        toggleOptical2.sampleHue = wrongHue;
      }
      if(elapsed == 200){
        requireTurning(toggleLeft);
      }
      if(elapsed == 220){
        toggleOptical1.sampleHue = toggleOptical2.sampleHue = targetHue;
      }
      if(elapsed == 260){
        toggleOptical2.nearby = false;
      }
      if(elapsed == 280){
        requireTurning(toggleLeft);
        toggleOptical2.nearby = true;
      }
    };
    require(activeToggle(competition, red), "Corrected toggle did not succeed");
    onWait = nullptr;
    require(fake::nowMs - startMs == 380,
            "Toggle completed without 100 ms of continuous target color and proximity");
    requireBraked(toggleLeft);
    requireBraked(toggleRight);
  }
  toggleOptical1.sampleHue = toggleOptical2.sampleHue = 220;
  const int timeoutStart = fake::nowMs;
  require(!activeToggle(competition, true), "Blue was accepted as red");
  require(fake::nowMs - timeoutStart == 3000, "Autonomous timeout changed");
  require(toggleLeft.commands.back().kind == "stop" &&
          toggleRight.commands.back().kind == "stop", "Timeout did not stop toggles");
  onWait = [&competition] { competition.enabled = false; };
  require(!activeToggle(competition, true), "Ended autonomous was ignored");
  onWait = nullptr;
  require(toggleLeft.commands.back().kind == "stop" &&
          toggleRight.commands.back().kind == "stop", "Ended autonomous did not stop toggles");
  competition.enabled = true;
  toggleOptical1.connected = false;
  require(!activeToggle(competition, true), "Missing sensor was ignored");
  toggleOptical1.connected = true;

  redAlliance = true;
  Controller.ButtonA.pressed = true;
  Controller.ButtonX.pressed = false;
  motorizedToggleing(true);
  toggleOptical1.sampleHue = toggleOptical2.sampleHue = 0;
  motorizedToggleing();
  fake::nowMs += 100;
  motorizedToggleing();
  requireBraked(toggleLeft);
  requireBraked(toggleRight);
  toggleOptical2.sampleHue = 220;
  motorizedToggleing();
  requireTurning(toggleLeft);
  requireBraked(toggleRight);
  Controller.ButtonX.pressed = true;
  motorizedToggleing();
  requireTurning(toggleLeft);
  requireTurning(toggleRight);
  Controller.ButtonA.pressed = Controller.ButtonX.pressed = false;
  motorizedToggleing();
  require(toggleLeft.commands.back().kind == "stop" &&
          toggleRight.commands.back().kind == "stop", "Button release did not stop toggles");
}
}

int main() {
  try {
    Controller.Screen.lines[2] = "Right Wing Red";
    toggleOptical1.connected = toggleOptical2.connected = false;
    updateAt(0);
    require(Controller.Screen.printCalls == 2, "Missing sensors not initially displayed");
    require(Controller.Screen.lines[0] == "L: sensor missing   " &&
            Controller.Screen.lines[1] == "R: sensor missing   ", "Missing-sensor text incorrect");
    updateAt(99);
    updateAt(100);
    require(Controller.Screen.printCalls == 2, "Unchanged missing messages were redrawn");
    toggleOptical2.connected = true;
    toggleOptical2.sampleHue = 25.1;
    updateAt(199);
    require(Controller.Screen.printCalls == 2, "Optical refresh interval not respected");
    updateAt(200);
    require(Controller.Screen.printCalls == 3 &&
            Controller.Screen.lines[0] == "L H: 25 Y Near:N    ", "Reconnection not displayed");
    toggleOptical2.sampleHue = 25.2;
    updateAt(300);
    require(Controller.Screen.printCalls == 3, "Unchanged rounded hue was redrawn");
    toggleOptical2.sampleHue = 26;
    updateAt(400);
    require(Controller.Screen.printCalls == 4 &&
            Controller.Screen.lines[0] == "L H: 26 Y Near:N    ", "Hue change not displayed");
    toggleOptical2.nearby = true;
    updateAt(500);
    require(Controller.Screen.printCalls == 5 &&
            Controller.Screen.lines[0] == "L H: 26 Y Near:Y    ", "Proximity change not displayed");
    toggleOptical2.connected = false;
    updateAt(600);
    require(Controller.Screen.printCalls == 6 &&
            Controller.Screen.lines[0] == "L: sensor missing   ", "Disconnection not displayed");
    toggleOptical2.connected = true;
    updateAt(700);
    require(Controller.Screen.printCalls == 7, "Second reconnection not displayed");
    require(Controller.Screen.clearCalls == 0, "Optical redraws clear and flicker");
    require(Controller.Screen.lines[2] == "Right Wing Red", "Optical display overwrote selection");
    require(toggleLeft.commands.empty() && toggleRight.commands.empty(),
            "Optical display commanded motors");
    checkActiveToggle();
    std::cout << "PASS optical display and stable-color toggle correction\n";
  } catch (const std::exception& error) {
    std::cerr << "FAIL optical display: " << error.what() << '\n';
    return 1;
  }
}
