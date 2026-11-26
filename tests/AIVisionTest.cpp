#include "vex.h"
#include "AIVision.h"

#include <iostream>

namespace fake {
int nowMs = 0;
}

const vex::aivision::aiobjdesc vex::aivision::ALL_AIOBJS;
vex::brain Brain;
vex::aivision AIVision;

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool hasText(const std::string& text) {
  for (const auto& line : Brain.Screen.lines) {
    if (line.text == text) {
      return true;
    }
  }
  return false;
}

vex::aivision::object cup() {
  vex::aivision::object object;
  object.id = 4;
  object.type = vex::aivision::objectType::modelObject;
  object.exists = true;
  object.className = "Cup";
  object.score = 79;
  object.centerX = 152;
  object.centerY = 153;
  return object;
}

void missingSensor() {
  AIVision.connected = false;
  updateAIVisionDisplay();
  require(hasText("AI Vision: sensor missing on port 6"), "Missing sensor not displayed");
  require(AIVision.snapshotCalls == 0, "Missing sensor must not be sampled");
  require(AIVision.configurationCalls == 0, "Missing sensor must not be configured");
}

void configuration() {
  updateAIVisionDisplay();
  require(AIVision.modelEnabled, "Model detection not enabled");
  require(!AIVision.colorsEnabled && !AIVision.tagsEnabled, "Other detection modes not disabled");
  require(AIVision.requestedObjects == 24, "Snapshot must request 24 objects");
  require(hasText("Filter: ALL model classes"), "Default filter not displayed");
  require(hasText("No matching pre-trained AI objects."), "Empty snapshot not displayed");
  fake::nowMs = 250;
  updateAIVisionDisplay();
  require(AIVision.configurationCalls == 1, "Sensor reconfigured without a disconnect");
}

void recognition() {
  AIVision.objects.data[0] = cup();
  auto pin = cup();
  pin.className = "RedYellowPin";
  pin.id = 2;
  pin.score = 85;
  pin.centerX = 159;
  pin.centerY = 73;
  AIVision.objects.data[1] = pin;
  AIVision.snapshotResult = 2;
  updateAIVisionDisplay();
  require(hasText("AI Vision P6: 2 detected, 2 matched"), "Incorrect detection counts");
  require(hasText("Cup / 4 / 79 / 152,153"), "Cup data not displayed accurately");
  require(hasText("RedYellowPin / 2 / 85 / 159,73"), "Pin data not displayed accurately");
  require(hasText("Showing 2/2 matches; display only"), "Incorrect match summary");
}

void objectTypes() {
  const vex::aivision::objectType types[] = {
    vex::aivision::objectType::unknownObject,
    vex::aivision::objectType::colorObject,
    vex::aivision::objectType::codeObject,
    vex::aivision::objectType::tagObject
  };
  for (std::size_t i = 0; i < 4; ++i) {
    AIVision.objects.data[i] = cup();
    AIVision.objects.data[i].type = types[i];
  }
  AIVision.snapshotResult = 4;
  updateAIVisionDisplay();
  require(hasText("AI Vision P6: 4 detected, 0 matched"), "Non-model object counted as a match");
  require(hasText("No matching pre-trained AI objects."), "No-match message missing");
}

void nonexistentObject() {
  AIVision.objects.data[0] = cup();
  AIVision.objects.data[0].exists = false;
  AIVision.snapshotResult = 1;
  updateAIVisionDisplay();
  require(hasText("AI Vision P6: 1 detected, 0 matched"), "Nonexistent object counted");
}

void removal() {
  AIVision.objects.data[0] = cup();
  AIVision.snapshotResult = 1;
  updateAIVisionDisplay();
  require(hasText("Cup / 4 / 79 / 152,153"), "Initial object missing");
  AIVision.snapshotResult = 0;
  fake::nowMs = 250;
  updateAIVisionDisplay();
  require(hasText("AI Vision P6: 0 detected, 0 matched"), "Removed object still counted");
  require(!hasText("Cup / 4 / 79 / 152,153"), "Stale object remains on screen");
}

void displayLimit() {
  for (auto& object : AIVision.objects.data) {
    object = cup();
  }
  AIVision.snapshotResult = 24;
  updateAIVisionDisplay();
  int displayed = 0;
  for (const auto& line : Brain.Screen.lines) {
    if (line.y >= 96 && line.y <= 196) {
      ++displayed;
    }
  }
  require(displayed == 6, "Display must contain exactly 6 object rows");
  require(hasText("AI Vision P6: 24 detected, 24 matched"), "Maximum snapshot count incorrect");
  require(hasText("Showing 6/24 matches; display only"), "Display truncation not reported");
}

void snapshotError(int count) {
  AIVision.snapshotResult = count;
  updateAIVisionDisplay();
  require(hasText("AI Vision: snapshot error (" + std::to_string(count) + ")"),
          "Snapshot error not displayed");
  require(AIVision.objects.reads == 0, "Invalid snapshot accessed the object array");
  AIVision.snapshotResult = 0;
  fake::nowMs = 250;
  updateAIVisionDisplay();
  require(hasText("AI Vision P6: 0 detected, 0 matched"), "Snapshot did not recover");
}

void negativeSnapshot() { snapshotError(-1); }
void excessiveSnapshot() { snapshotError(25); }

void refreshInterval() {
  updateAIVisionDisplay();
  fake::nowMs = 249;
  updateAIVisionDisplay();
  require(AIVision.snapshotCalls == 1 && Brain.Screen.clearCalls == 1,
          "Display refreshed before 250 ms");
  fake::nowMs = 250;
  updateAIVisionDisplay();
  require(AIVision.snapshotCalls == 2, "Display did not refresh at 250 ms");
  fake::nowMs = 499;
  updateAIVisionDisplay();
  require(AIVision.snapshotCalls == 2, "Timer did not reset after refresh");
  fake::nowMs = 500;
  updateAIVisionDisplay();
  require(AIVision.snapshotCalls == 3, "Second refresh did not occur at 500 ms");
}

void reconnection() {
  updateAIVisionDisplay();
  AIVision.connected = false;
  fake::nowMs = 250;
  updateAIVisionDisplay();
  require(AIVision.snapshotCalls == 1, "Disconnected sensor sampled");
  require(hasText("AI Vision: sensor missing on port 6"), "Disconnect not displayed");
  AIVision.connected = true;
  AIVision.modelEnabled = false;
  fake::nowMs = 500;
  updateAIVisionDisplay();
  require(AIVision.configurationCalls == 2 && AIVision.modelEnabled,
          "Reconnected sensor not reconfigured");
  require(AIVision.snapshotCalls == 2, "Sampling did not resume");
}

struct testCase {
  const char* name;
  void (*run)();
};

const testCase tests[] = {
  {"missing-sensor", missingSensor},
  {"configuration", configuration},
  {"recognition", recognition},
  {"object-types", objectTypes},
  {"nonexistent-object", nonexistentObject},
  {"removal", removal},
  {"display-limit", displayLimit},
  {"negative-snapshot", negativeSnapshot},
  {"excessive-snapshot", excessiveSnapshot},
  {"refresh-interval", refreshInterval},
  {"reconnection", reconnection}
};
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: ai-vision-tests --list|TEST_NAME\n";
    return 1;
  }
  if (std::strcmp(argv[1], "--list") == 0) {
    for (const auto& test : tests) {
      std::cout << test.name << '\n';
    }
    return 0;
  }
  for (const auto& test : tests) {
    if (std::strcmp(argv[1], test.name) != 0) {
      continue;
    }
    try {
      test.run();
      std::cout << "PASS " << test.name << '\n';
      return 0;
    } catch (const std::exception& error) {
      std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
      return 1;
    }
  }
  std::cerr << "Unknown test: " << argv[1] << '\n';
  return 1;
}
