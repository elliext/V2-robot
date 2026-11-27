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
#if defined(AI_VISION_TEST)
constexpr int kRefreshMs = 1000;
#else
constexpr int kRefreshMs = 250;
#endif

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
  object.originX = 100;
  object.originY = 30;
  object.width = 80;
  object.height = 160;
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
  fake::nowMs = kRefreshMs;
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
  require(hasText("80x160 px / range N/A: uncalibrated"), "Uncalibrated range not explicit");
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
  fake::nowMs = kRefreshMs;
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
  require(displayed == 6, "Display must contain exactly 3 two-line object rows");
  require(hasText("AI Vision P6: 24 detected, 24 matched"), "Maximum snapshot count incorrect");
  require(hasText("Showing 3/24 matches; display only"), "Display truncation not reported");
}

void snapshotError(int count) {
  AIVision.snapshotResult = count;
  updateAIVisionDisplay();
  require(hasText("AI Vision: snapshot error (" + std::to_string(count) + ")"),
          "Snapshot error not displayed");
  require(AIVision.objects.reads == 0, "Invalid snapshot accessed the object array");
  AIVision.snapshotResult = 0;
  fake::nowMs = kRefreshMs;
  updateAIVisionDisplay();
  require(hasText("AI Vision P6: 0 detected, 0 matched"), "Snapshot did not recover");
}

void negativeSnapshot() { snapshotError(-1); }
void excessiveSnapshot() { snapshotError(25); }

void refreshInterval() {
  updateAIVisionDisplay();
  fake::nowMs = kRefreshMs - 1;
  updateAIVisionDisplay();
  require(AIVision.snapshotCalls == 1 && Brain.Screen.clearCalls == 1,
          "Display refreshed before the configured interval");
  fake::nowMs = kRefreshMs;
  updateAIVisionDisplay();
  require(AIVision.snapshotCalls == 2, "Display did not refresh at the configured interval");
  fake::nowMs = 2 * kRefreshMs - 1;
  updateAIVisionDisplay();
  require(AIVision.snapshotCalls == 2, "Timer did not reset after refresh");
  fake::nowMs = 2 * kRefreshMs;
  updateAIVisionDisplay();
  require(AIVision.snapshotCalls == 3, "Second refresh did not occur at twice the interval");
}

void reconnection() {
  updateAIVisionDisplay();
  AIVision.connected = false;
  fake::nowMs = kRefreshMs;
  updateAIVisionDisplay();
  require(AIVision.snapshotCalls == 1, "Disconnected sensor sampled");
  require(hasText("AI Vision: sensor missing on port 6"), "Disconnect not displayed");
  AIVision.connected = true;
  AIVision.modelEnabled = false;
  fake::nowMs = 2 * kRefreshMs;
  updateAIVisionDisplay();
  require(AIVision.configurationCalls == 2 && AIVision.modelEnabled,
          "Reconnected sensor not reconfigured");
  require(AIVision.snapshotCalls == 2, "Sampling did not resume");
}

void nextFrame() {
  fake::nowMs += kRefreshMs;
  updateAIVisionDisplay();
}

void tapCalibration() {
  Brain.Screen.touchDown = false;
  updateAIVisionDisplay();
  Brain.Screen.touchX = 310;
  Brain.Screen.touchY = 40;
  Brain.Screen.touchDown = true;
  nextFrame();
}

void calibratedCup() {
  AIVision.objects.data[0] = cup();
  AIVision.snapshotResult = 1;
  updateAIVisionDisplay();
  tapCalibration();
  require(hasText("Cal OK; approximate camera-axis range"), "Calibration not confirmed");
  require(hasText("Cal 812.8mm"), "Calibration button must show exact reference");
  require(hasText("80x160 px / approx 813 mm"), "Calibration reference must round to 813 mm");
}

void distanceScaling() {
  calibratedCup();
  AIVision.objects.data[0].width = 40;
  AIVision.objects.data[0].height = 80;
  nextFrame();
  require(hasText("40x80 px / approx 1626 mm"), "Half apparent length must double range");
  AIVision.objects.data[0].width = 100;
  AIVision.objects.data[0].height = 200;
  nextFrame();
  require(hasText("100x200 px / approx 650 mm"), "Larger apparent length must reduce range");
  AIVision.snapshotResult = 0;
  nextFrame();
  require(!hasText("100x200 px / approx 650 mm"), "Stale range remains after removal");
}

void sidewaysAndPin() {
  calibratedCup();
  auto& object = AIVision.objects.data[0];
  object.width = 160;
  object.height = 80;
  nextFrame();
  require(hasText("160x80 px / approx 813 mm"), "Sideways object must use width");
  object.className = "RedYellowPin";
  nextFrame();
  require(hasText("160x80 px / approx 816 mm"), "Pin must use its own known length");
  tapCalibration();
  require(hasText("160x80 px / approx 813 mm"), "Pin calibration must use pin length");
}

void invalidSizes() {
  calibratedCup();
  auto& object = AIVision.objects.data[0];
  object.width = 0;
  nextFrame();
  require(hasText("0x160 px / range N/A: invalid size"), "Zero width accepted");
  tapCalibration();
  require(hasText("Cal failed: invalid size or edge/clipped"), "Invalid calibration accepted");
  object.width = 80;
  object.height = -1;
  nextFrame();
  require(hasText("80x-1 px / range N/A: invalid size"), "Negative height accepted");
  object = cup();
  nextFrame();
  require(hasText("80x160 px / approx 813 mm"), "Failed calibration changed reference");
}

void clippedObjects() {
  calibratedCup();
  auto& object = AIVision.objects.data[0];
  const int origins[][2] = {{0, 30}, {100, 0}, {240, 30}, {100, 80}, {-1, 30}};
  for (const auto& origin : origins) {
    object.originX = origin[0];
    object.originY = origin[1];
    nextFrame();
    require(hasText("80x160 px / range N/A: edge/clipped"), "Frame-edge object accepted");
    tapCalibration();
    require(hasText("Cal failed: invalid size or edge/clipped"), "Clipped calibration accepted");
  }
  object = cup();
  nextFrame();
  require(hasText("80x160 px / approx 813 mm"), "Clipped calibration changed reference");
}

void calibrationCandidates() {
  updateAIVisionDisplay();
  tapCalibration();
  require(hasText("Cal failed: need exactly ONE Cup or pin"), "Empty calibration accepted");
  AIVision.objects.data[0] = cup();
  AIVision.objects.data[1] = cup();
  AIVision.snapshotResult = 2;
  tapCalibration();
  require(hasText("Cal failed: need exactly ONE Cup or pin"), "Ambiguous calibration accepted");
  require(hasText("80x160 px / range N/A: uncalibrated"), "Ambiguous calibration produced range");
  AIVision.snapshotResult = 1;
  AIVision.objects.data[0].className = "Unknown";
  tapCalibration();
  require(hasText("80x160 px / range N/A: unknown size"), "Unsupported class received range");
  require(hasText("Cal failed: need exactly ONE Cup or pin"), "Unknown class calibrated");
  AIVision.objects.data[0] = cup();
  AIVision.objects.data[0].type = vex::aivision::objectType::colorObject;
  tapCalibration();
  require(hasText("Cal failed: need exactly ONE Cup or pin"), "Non-model object calibrated");
}

void calibrationTouch() {
  AIVision.objects.data[0] = cup();
  AIVision.snapshotResult = 1;
  updateAIVisionDisplay();
  Brain.Screen.touchDown = true;
  Brain.Screen.touchX = 299;
  Brain.Screen.touchY = 40;
  nextFrame();
  require(hasText("80x160 px / range N/A: uncalibrated"), "Outside button calibrated");
  tapCalibration();
  AIVision.objects.data[0].width = 40;
  AIVision.objects.data[0].height = 80;
  nextFrame();
  require(hasText("40x80 px / approx 1626 mm"), "Held touch recalibrated");
  Brain.Screen.touchDown = false;
  updateAIVisionDisplay();
  Brain.Screen.touchDown = true;
  fake::nowMs += 10;
  updateAIVisionDisplay();
  Brain.Screen.touchDown = false;
  fake::nowMs += 10;
  updateAIVisionDisplay();
  nextFrame();
  require(hasText("40x80 px / approx 813 mm"), "Tap between snapshots was lost");
}

void calibrationDisconnect() {
  calibratedCup();
  AIVision.connected = false;
  nextFrame();
  AIVision.connected = true;
  nextFrame();
  require(hasText("80x160 px / range N/A: uncalibrated"), "Disconnect retained calibration");
}

void calibrationSnapshotFault() {
  calibratedCup();
  AIVision.snapshotResult = -1;
  tapCalibration();
  require(hasText("AI Vision: snapshot error (-1)"), "Snapshot fault hidden by calibration");
  AIVision.snapshotResult = 1;
  AIVision.objects.data[0].width = 40;
  AIVision.objects.data[0].height = 80;
  nextFrame();
  require(hasText("40x80 px / approx 1626 mm"), "Faulted tap recalibrated on recovery");
}

#if defined(AI_VISION_TEST)
void automaticCalibration() {
  AIVision.objects.data[0] = cup();
  AIVision.objects.data[1] = cup();
  AIVision.objects.data[1].className = "RedYellowPin";
  AIVision.snapshotResult = 2;
  updateAIVisionDisplay();
  for (int i = 0; i < 3; ++i) {
    nextFrame();
  }
  require(hasText("80x160 px / range N/A: uncalibrated"), "Auto calibrated before five samples");
  nextFrame();
  require(hasText("Auto Cal OK; rough 32in reference"), "Auto calibration failed with a pin present");
  require(hasText("80x160 px / approx 813 mm"), "Auto reference incorrect");
  AIVision.objects.data[0].height = 80;
  AIVision.objects.data[0].width = 40;
  for (int i = 0; i < 6; ++i) {
    nextFrame();
  }
  require(hasText("40x80 px / approx 1626 mm"), "Auto calibration repeated after moving cup");
  AIVision.connected = false;
  nextFrame();
  AIVision.connected = true;
  for (int i = 0; i < 6; ++i) {
    nextFrame();
  }
  require(hasText("40x80 px / range N/A: uncalibrated"), "Disconnect rearmed completed auto calibration");
}

void automaticStability() {
  AIVision.objects.data[0] = cup();
  AIVision.objects.data[0].originY = 10;
  AIVision.snapshotResult = 1;
  updateAIVisionDisplay();
  nextFrame();
  AIVision.objects.data[0].height = 200;
  nextFrame();
  require(hasText("Auto Cal 812.8mm: Cup stable 1/5"), "Unstable sample did not restart sequence");
  AIVision.objects.data[0].height = 210;
  for (int i = 0; i < 3; ++i) {
    nextFrame();
  }
  require(hasText("80x210 px / range N/A: uncalibrated"), "Unstable sequence calibrated too soon");
  nextFrame();
  require(hasText("80x210 px / approx 805 mm"), "Auto calibration did not average five lengths");
}

void automaticReset() {
  AIVision.objects.data[0] = cup();
  AIVision.snapshotResult = 1;
  updateAIVisionDisplay();
  nextFrame();
  AIVision.objects.data[1] = cup();
  AIVision.snapshotResult = 2;
  nextFrame();
  require(hasText("Auto Cal waiting: need exactly ONE Cup"), "Duplicate cups accepted");
  AIVision.snapshotResult = 1;
  nextFrame();
  AIVision.objects.data[0].originX = 0;
  nextFrame();
  require(hasText("Auto Cal waiting: invalid or clipped Cup"), "Clipped cup accepted");
  AIVision.objects.data[0] = cup();
  nextFrame();
  AIVision.snapshotResult = -1;
  nextFrame();
  AIVision.snapshotResult = 1;
  nextFrame();
  require(hasText("Auto Cal 812.8mm: Cup stable 1/5"), "Snapshot fault retained progress");
  AIVision.snapshotResult = 0;
  nextFrame();
  AIVision.snapshotResult = 1;
  nextFrame();
  require(hasText("Auto Cal 812.8mm: Cup stable 1/5"), "Missing cup retained progress");
  AIVision.connected = false;
  nextFrame();
  AIVision.connected = true;
  nextFrame();
  require(hasText("Auto Cal 812.8mm: Cup stable 1/5"), "Disconnect retained partial progress");
}
#endif

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
  {"reconnection", reconnection},
  {"distance-scaling", distanceScaling},
  {"sideways-and-pin", sidewaysAndPin},
  {"invalid-sizes", invalidSizes},
  {"clipped-objects", clippedObjects},
  {"calibration-candidates", calibrationCandidates},
  {"calibration-touch", calibrationTouch},
  {"calibration-disconnect", calibrationDisconnect},
  {"calibration-snapshot-fault", calibrationSnapshotFault},
#if defined(AI_VISION_TEST)
  {"automatic-calibration", automaticCalibration},
  {"automatic-stability", automaticStability},
  {"automatic-reset", automaticReset}
#endif
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
