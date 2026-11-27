#include "vex.h"
#include "AIVision.h"
#include "AprilTag.h"

using namespace vex;

namespace {
#if defined(AI_VISION_TEST)
constexpr int kRefreshIntervalMs = 1000;
#else
constexpr int kRefreshIntervalMs = 250;
#endif
constexpr int kSnapshotObjects = 24;
constexpr int kDisplayObjects = 3;
constexpr int kDisplayTags = 4;
constexpr double kCalibrationDistanceMm = 812.8;
constexpr int kImageWidth = 320;
constexpr int kImageHeight = 240;
// Empty means all model classes; otherwise use an exact reported class name.
constexpr char kTargetClassName[] = "";

double objectLengthMm(const aivision::object& object) {
  if (strcmp(object.className, "Cup") == 0) {
    return 164.5;
  }
  if (strcmp(object.className, "RedYellowPin") == 0) {
    return 165.1;
  }
  return 0;
}

const char* sizeError(const aivision::object& object) {
  if (object.width <= 0 || object.height <= 0) {
    return "invalid size";
  }
  if (object.originX <= 0 || object.originY <= 0 ||
      object.originX + object.width >= kImageWidth ||
      object.originY + object.height >= kImageHeight) {
    return "edge/clipped";
  }
  return nullptr;
}

int apparentLength(const aivision::object& object) {
  // The long axis may be vertical or horizontal; tilt is not recoverable from a box.
  return object.width > object.height ? object.width : object.height;
}

bool matchesTarget(const aivision::object& object) {
  return object.exists && object.type == aivision::objectType::modelObject &&
         (kTargetClassName[0] == '\0' ||
          strcmp(object.className, kTargetClassName) == 0);
}

#if defined(AI_VISION_TEST)
struct autoCalibration {
  bool finished = false;
  int samples = 0;
  int minimum = 0;
  int maximum = 0;
  int total = 0;
  bool horizontal = false;

  void reset() {
    samples = 0;
    total = 0;
  }

  void sample(const aivision::object& object) {
    const int length = apparentLength(object);
    const bool currentHorizontal = object.width > object.height;
    const int newMinimum = samples == 0 || length < minimum ? length : minimum;
    const int newMaximum = samples == 0 || length > maximum ? length : maximum;
    if (samples > 0 && (currentHorizontal != horizontal ||
                       newMaximum > newMinimum * 1.10)) {
      reset();
    }
    minimum = samples == 0 || length < minimum ? length : minimum;
    maximum = samples == 0 || length > maximum ? length : maximum;
    horizontal = currentHorizontal;
    total += length;
    ++samples;
  }
};
#endif
}

void updateAIVisionDisplay() {
  static timer refreshTimer;
  static bool firstUpdate = true;
  static bool configured = false;
  static bool missingReported = false;
  static bool snapshotFault = false;
  static bool touchHeld = false;
  static bool calibrationRequested = false;
  static double focalLengthPixels = 0;
  static const char* calibrationStatus = nullptr;
#if defined(AI_VISION_TEST)
  static autoCalibration automatic;
#endif
  const bool touching = Brain.Screen.pressing();
  if (touching && !touchHeld &&
      Brain.Screen.xPosition() >= 300 && Brain.Screen.xPosition() < 470 &&
      Brain.Screen.yPosition() >= 32 && Brain.Screen.yPosition() < 58) {
    calibrationRequested = true;
  }
  touchHeld = touching;
  if (!firstUpdate && refreshTimer.time(msec) < kRefreshIntervalMs) {
    return;
  }
  firstUpdate = false;
  refreshTimer.reset();

  if (!AIVision.installed()) {
    configured = false;
    focalLengthPixels = 0;
    calibrationRequested = false;
    calibrationStatus = nullptr;
#if defined(AI_VISION_TEST)
    automatic.reset();
#endif
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
    AIVision.tagDetection(true);
    AIVision.modelDetection(true);
    configured = true;
    printf("AI Vision: sensor connected on port 6; model and AprilTag detection enabled.\n");
  }

  const int count = AIVision.takeSnapshot(aivision::ALL_AIOBJS, kSnapshotObjects);
  Brain.Screen.clearScreen();
  if (count < 0 || count > kSnapshotObjects) {
    calibrationRequested = false;
#if defined(AI_VISION_TEST)
    automatic.reset();
#endif
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
  int calibrationCandidates = 0;
  int calibrationIndex = -1;
#if defined(AI_VISION_TEST)
  int cupCandidates = 0;
  int cupIndex = -1;
#endif
  for (int i = 0; i < count; ++i) {
    if (matchesTarget(AIVision.objects[i])) {
      ++matches;
      if (objectLengthMm(AIVision.objects[i]) > 0) {
        ++calibrationCandidates;
        calibrationIndex = i;
      }
#if defined(AI_VISION_TEST)
      if (strcmp(AIVision.objects[i].className, "Cup") == 0) {
        ++cupCandidates;
        cupIndex = i;
      }
#endif
    }
  }

#if defined(AI_VISION_TEST)
  if (!automatic.finished && !calibrationRequested) {
    if (cupCandidates != 1) {
      automatic.reset();
      calibrationStatus = "Auto Cal waiting: need exactly ONE Cup";
    } else if (sizeError(AIVision.objects[cupIndex]) != nullptr) {
      automatic.reset();
      calibrationStatus = "Auto Cal waiting: invalid or clipped Cup";
    } else {
      automatic.sample(AIVision.objects[cupIndex]);
      calibrationStatus = nullptr;
      if (automatic.samples == 5) {
        const double meanPixels = automatic.total / 5.0;
        focalLengthPixels = meanPixels * kCalibrationDistanceMm /
                            objectLengthMm(AIVision.objects[cupIndex]);
        automatic.finished = true;
        calibrationStatus = "Auto Cal OK; rough 32in reference";
        printf("AI Vision: auto-calibrated Cup assuming %.1f mm; mean %.2f px, focal %.2f px. Reference distance is approximate.\n",
               kCalibrationDistanceMm, meanPixels, focalLengthPixels);
      }
    }
  }
#endif

  if (calibrationRequested) {
    calibrationRequested = false;
    if (calibrationCandidates != 1) {
      calibrationStatus = "Cal failed: need exactly ONE Cup or pin";
      printf("AI Vision: calibration rejected; %d supported objects. Previous calibration unchanged.\n",
             calibrationCandidates);
    } else {
      const auto& object = AIVision.objects[calibrationIndex];
      const char* error = sizeError(object);
      if (error != nullptr) {
        calibrationStatus = "Cal failed: invalid size or edge/clipped";
        printf("AI Vision: calibration rejected: %s. Previous calibration unchanged.\n", error);
      } else {
        focalLengthPixels = apparentLength(object) * kCalibrationDistanceMm /
                            objectLengthMm(object);
        calibrationStatus = "Cal OK; approximate camera-axis range";
#if defined(AI_VISION_TEST)
        automatic.finished = true;
        automatic.reset();
#endif
        printf("AI Vision: calibrated %s at %.1f mm, %d px; focal length %.2f px.\n",
               object.className, kCalibrationDistanceMm, apparentLength(object),
               focalLengthPixels);
      }
    }
  }

  Brain.Screen.printAt(10, 24, true, "AI Vision P6: %d detected, %d matched", count, matches);
  Brain.Screen.printAt(10, 48, true, "Filter: %.19s",
                       kTargetClassName[0] == '\0' ? "ALL model classes" : kTargetClassName);
  Brain.Screen.drawRectangle(300, 32, 170, 26);
  Brain.Screen.printAt(310, 51, true, "Cal %.1fmm", kCalibrationDistanceMm);
  if (calibrationStatus != nullptr) {
    Brain.Screen.printAt(10, 72, true, "%s", calibrationStatus);
  } else {
#if defined(AI_VISION_TEST)
    if (!automatic.finished) {
      Brain.Screen.printAt(10, 72, true, "Auto Cal %.1fmm: Cup stable %d/5",
                           kCalibrationDistanceMm, automatic.samples);
    } else
#endif
    {
      Brain.Screen.printAt(10, 72, true, "ONE upright object at %.1fmm; tap Cal",
                           kCalibrationDistanceMm);
    }
  }
  int displayed = 0;
  for (int i = 0; i < count && displayed < kDisplayObjects; ++i) {
    const auto& object = AIVision.objects[i];
    if (!matchesTarget(object)) {
      continue;
    }
    const int y = 96 + displayed * 40;
    Brain.Screen.printAt(10, y, true, "%.19s / %d / %d / %d,%d",
                         object.className, static_cast<int>(object.id), object.score,
                         object.centerX, object.centerY);
    const char* unavailable = nullptr;
    const double lengthMm = objectLengthMm(object);
    if (lengthMm == 0) {
      unavailable = "unknown size";
    } else if (sizeError(object) != nullptr) {
      unavailable = sizeError(object);
    } else if (focalLengthPixels == 0) {
      unavailable = "uncalibrated";
    }
    if (unavailable != nullptr) {
      Brain.Screen.printAt(10, y + 20, true, "%dx%d px / range N/A: %s",
                           object.width, object.height, unavailable);
    } else {
      const double distanceMm = lengthMm * focalLengthPixels / apparentLength(object);
      Brain.Screen.printAt(10, y + 20, true, "%dx%d px / approx %.0f mm",
                           object.width, object.height, distanceMm);
    }
    ++displayed;
  }
  if (matches == 0) {
    Brain.Screen.printAt(10, 96, true, "No matching pre-trained AI objects.");
  }
  // Must follow every model-object read: the tag snapshot overwrites AIVision.objects.
  GoalTagDetection tags[24];
  const int tagCount = detectGoalTags(tags, 24);
  if (tagCount < 0) {
    Brain.Screen.printAt(10, 208, true, "Tags: snapshot error");
  } else if (tagCount == 0) {
    Brain.Screen.printAt(10, 208, true, "Tags: none");
  } else {
    char line[64];
    int length = snprintf(line, sizeof(line), "Tags %d:", tagCount);
    for (int i = 0; i < tagCount && i < kDisplayTags; ++i) {
      const GoalTag& goal = tags[i].goal;
      if (goal.role == GoalRole::Center) {
        length += snprintf(line + length, sizeof(line) - length, " %d=C", tags[i].tagId);
      } else {
        length += snprintf(line + length, sizeof(line) - length, " %d=%c%d", tags[i].tagId,
                           goal.role == GoalRole::Alliance ? 'A' : 'N', goal.quadrantNumber);
      }
    }
    Brain.Screen.printAt(10, 208, true, "%s", line);
  }
  Brain.Screen.printAt(10, 228, true, "Showing %d/%d matches; display only", displayed, matches);
}
