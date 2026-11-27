#include "vex.h"
#include "Toggle.h"

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

void vex::wait(int duration, vex::timeUnits) {
  fake::nowMs += duration;
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
}

int main() {
  try {
    Controller.Screen.lines[2] = "Setup Red Left";
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
    require(Controller.Screen.lines[2] == "Setup Red Left", "Optical display overwrote match setup");
    require(toggleLeft.commands.empty() && toggleRight.commands.empty(),
            "Optical display commanded motors");
    std::cout << "PASS optical display change-only rendering and selection preservation\n";
  } catch (const std::exception& error) {
    std::cerr << "FAIL optical display: " << error.what() << '\n';
    return 1;
  }
}
