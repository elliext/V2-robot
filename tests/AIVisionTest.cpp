#include "vex.h"
#include "AIVision.h"
#include "AprilTag.h"

#include <initializer_list>
#include <iostream>

namespace fake {
int nowMs = 0;
}

const vex::aivision::aiobjdesc vex::aivision::ALL_AIOBJS;
const vex::aivision::aiobjdesc vex::aivision::ALL_TAGS;
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
  require(!AIVision.colorsEnabled && AIVision.tagsEnabled, "Only model and tag detection must be enabled");
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

vex::aivision::object tag(int id) {
  vex::aivision::object object;
  object.id = id;
  object.type = vex::aivision::objectType::tagObject;
  object.exists = true;
  object.centerX = 100 + id;
  object.centerY = 90;
  object.width = 30;
  object.height = 31;
  return object;
}

void setTags(std::initializer_list<int> ids) {
  std::size_t index = 0;
  for (const int id : ids) {
    AIVision.tagData[index++] = tag(id);
  }
  AIVision.tagSnapshotResult = static_cast<int>(ids.size());
}

void goalClassification() {
  const struct {
    int id;
    GoalRole role;
    int quadrant;
  } expected[] = {
    {0, GoalRole::Center, 0},
    {1, GoalRole::Neutral, 1},
    {2, GoalRole::Alliance, 1},
    {3, GoalRole::Alliance, 2},
    {4, GoalRole::Neutral, 2},
    {5, GoalRole::Neutral, 1},
    {6, GoalRole::Alliance, 1},
    {7, GoalRole::Alliance, 2},
    {8, GoalRole::Neutral, 2},
  };
  for (const auto& row : expected) {
    GoalTag goal;
    require(classifyGoalTag(row.id, goal), "Goal tag rejected");
    require(goal.role == row.role && goal.quadrantNumber == row.quadrant,
            "Tag classified as the wrong goal");
  }
  GoalTag goal;
  require(!classifyGoalTag(-1, goal) && !classifyGoalTag(9, goal),
          "Non-goal tag accepted");
}

void goalQuadrantNames() {
  GoalTag goal;
  classifyGoalTag(2, goal);
  require(std::strcmp(goalQuadrantName(goal, true), "Red 1") == 0, "Tag 2 is Red 1");
  require(std::strcmp(goalQuadrantName(goal, false), "Blue 1") == 0, "Tag 2 is Blue 1");
  classifyGoalTag(4, goal);
  require(std::strcmp(goalQuadrantName(goal, true), "Red 2") == 0, "Tag 4 is Red 2");
  require(std::strcmp(goalQuadrantName(goal, false), "Blue 2") == 0, "Tag 4 is Blue 2");
  classifyGoalTag(0, goal);
  require(std::strcmp(goalQuadrantName(goal, true), "Center") == 0, "Tag 0 is the center");
}

void tagDetection() {
  setTags({3, 0});
  GoalTagDetection found[4];
  require(detectGoalTags(found, 4) == 2, "Both tags not detected");
  require(AIVision.requestedObjects == 0, "Tag snapshot must not use the model snapshot");
  require(found[0].tagId == 3 && found[0].goal.role == GoalRole::Alliance &&
          found[0].goal.quadrantNumber == 2, "Tag 3 misclassified");
  require(found[0].centerX == 103 && found[0].centerY == 90 &&
          found[0].width == 30 && found[0].height == 31, "Tag geometry not copied");
  require(found[1].tagId == 0 && found[1].goal.role == GoalRole::Center, "Tag 0 misclassified");
}

void tagFiltering() {
  AIVision.tagData[0] = tag(9);
  AIVision.tagData[1] = tag(2);
  AIVision.tagData[1].exists = false;
  AIVision.tagData[2] = tag(1);
  AIVision.tagData[2].type = vex::aivision::objectType::modelObject;
  AIVision.tagData[3] = tag(4);
  AIVision.tagSnapshotResult = 4;
  GoalTagDetection found[4];
  require(detectGoalTags(found, 4) == 1 && found[0].tagId == 4,
          "Unknown, missing or non-tag objects not filtered");
}

void tagOutputLimit() {
  setTags({1, 2, 3, 4});
  GoalTagDetection found[2];
  require(detectGoalTags(found, 2) == 2 && found[1].tagId == 2, "Output limit exceeded");
}

void tagSnapshotErrors() {
  GoalTagDetection found[1];
  AIVision.tagSnapshotResult = -1;
  require(detectGoalTags(found, 1) == -1, "Negative tag count accepted");
  AIVision.tagSnapshotResult = 25;
  require(detectGoalTags(found, 1) == -1, "Excessive tag count accepted");
}

void displayTags() {
  setTags({0, 1, 2, 3, 4});
  updateAIVisionDisplay();
  require(hasText("Tags 5: 0=C 1=N1 2=A1 3=A2"), "Tags not displayed");
  require(AIVision.tagSnapshotCalls == 1 && AIVision.snapshotCalls == 1,
          "Each refresh needs one model and one tag snapshot");
  AIVision.tagSnapshotResult = 0;
  nextFrame();
  require(hasText("Tags: none"), "Missing tags not reported");
  AIVision.tagSnapshotResult = -1;
  nextFrame();
  require(hasText("Tags: snapshot error"), "Tag snapshot error not displayed");
}

void displayModelAndTags() {
  AIVision.objects.data[0] = cup();
  AIVision.snapshotResult = 1;
  setTags({2});
  for (int frame = 0; frame < 2; ++frame) {
    nextFrame();
    require(hasText("Cup / 4 / 79 / 152,153"), "Cup lost when tags are also detected");
    require(hasText("AI Vision P6: 1 detected, 1 matched"), "Tag counted as a model object");
    require(hasText("Tags 1: 2=A1"), "Tag missing next to model object");
  }
}

void tagsOnMissingSensor() {
  AIVision.connected = false;
  setTags({2});
  updateAIVisionDisplay();
  require(AIVision.tagSnapshotCalls == 0, "Missing sensor must not take tag snapshots");
}

void tagsSkippedOnModelFault() {
  AIVision.snapshotResult = -1;
  setTags({2});
  updateAIVisionDisplay();
  require(AIVision.tagSnapshotCalls == 0, "Tag snapshot taken after a model snapshot fault");
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
  {"goal-classification", goalClassification},
  {"goal-quadrant-names", goalQuadrantNames},
  {"tag-detection", tagDetection},
  {"tag-filtering", tagFiltering},
  {"tag-output-limit", tagOutputLimit},
  {"tag-snapshot-errors", tagSnapshotErrors},
  {"display-tags", displayTags},
  {"display-model-and-tags", displayModelAndTags},
  {"tags-on-missing-sensor", tagsOnMissingSensor},
  {"tags-skipped-on-model-fault", tagsSkippedOnModelFault},
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
