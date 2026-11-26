#include "vex.h"
#include "AIVision.h"

using namespace vex;

namespace {
constexpr int kRefreshIntervalMs = 250;
constexpr int kSnapshotObjects = 24;
constexpr int kDisplayObjects = 6;
// Empty means all model classes; otherwise use an exact reported class name.
constexpr char kTargetClassName[] = "";

bool matchesTarget(const aivision::object& object) {
  return object.exists && object.type == aivision::objectType::modelObject &&
         (kTargetClassName[0] == '\0' ||
          strcmp(object.className, kTargetClassName) == 0);
}
}

void updateAIVisionDisplay() {
  static timer refreshTimer;
  static bool firstUpdate = true;
  static bool configured = false;
  static bool missingReported = false;
  static bool snapshotFault = false;
  if (!firstUpdate && refreshTimer.time(msec) < kRefreshIntervalMs) {
    return;
  }
  firstUpdate = false;
  refreshTimer.reset();

  if (!AIVision.installed()) {
    configured = false;
    if (!missingReported) {
      printf("AI Vision: sensor missing on Smart Port 6.\n");
      missingReported = true;
    }
    Brain.Screen.clearScreen();
    Brain.Screen.printAt(10, 30, true, "AI Vision: sensor missing on port 6");
    Brain.Screen.printAt(10, 60, true, "Check Brain power and Smart Cable.");
    return;
  }

  missingReported = false;
  if (!configured) {
    AIVision.colorDetection(false);
    AIVision.tagDetection(false);
    AIVision.modelDetection(true);
    configured = true;
    printf("AI Vision: sensor connected on port 6; model detection enabled.\n");
  }

  const int count = AIVision.takeSnapshot(aivision::ALL_AIOBJS, kSnapshotObjects);
  Brain.Screen.clearScreen();
  if (count < 0 || count > kSnapshotObjects) {
    if (!snapshotFault) {
      printf("AI Vision: invalid snapshot object count %d.\n", count);
    }
    snapshotFault = true;
    Brain.Screen.printAt(10, 30, true, "AI Vision: snapshot error (%d)", count);
    return;
  }
  if (snapshotFault) {
    printf("AI Vision: snapshot data recovered.\n");
    snapshotFault = false;
  }

  int matches = 0;
  for (int i = 0; i < count; ++i) {
    if (matchesTarget(AIVision.objects[i])) {
      ++matches;
    }
  }

  Brain.Screen.printAt(10, 24, true, "AI Vision P6: %d detected, %d matched", count, matches);
  Brain.Screen.printAt(10, 48, true, "Filter: %.19s",
                       kTargetClassName[0] == '\0' ? "ALL model classes" : kTargetClassName);
  Brain.Screen.printAt(10, 72, true, "Class / ID / score / center X,Y");
  int displayed = 0;
  for (int i = 0; i < count && displayed < kDisplayObjects; ++i) {
    const auto& object = AIVision.objects[i];
    if (!matchesTarget(object)) {
      continue;
    }
    Brain.Screen.printAt(10, 96 + displayed * 20, true, "%.19s / %d / %d / %d,%d",
                         object.className, static_cast<int>(object.id), object.score,
                         object.centerX, object.centerY);
    ++displayed;
  }
  if (matches == 0) {
    Brain.Screen.printAt(10, 96, true, "No matching pre-trained AI objects.");
  }
  Brain.Screen.printAt(10, 228, true, "Showing %d/%d matches; display only", displayed, matches);
}
