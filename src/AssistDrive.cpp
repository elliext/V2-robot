#include "vex.h"
#include "AssistDrive.h"
#include "AIVision.h"
#include "AutoAim.h"
#include "Drivetrain.h"

using namespace vex;

namespace {
constexpr int kSnapshotObjects = 24;
constexpr int kCancelStick = 50;  // Firm enough to ignore a thumb returning to the stick.

AutoAim assist;

Observation observe(AimTarget target) {
  AIVisionLock sensorLock;
  if (!AIVision.installed()) {
    return Observation();
  }
  if (target == AimTarget::Goal) {
    GoalTagDetection tags[kSnapshotObjects];
    const int count = detectGoalTags(tags, kSnapshotObjects);
    return count > 0 ? observeGoal(tags, count) : Observation();
  }
  const int count = AIVision.takeSnapshot(aivision::ALL_AIOBJS, kSnapshotObjects);
  if (count <= 0 || count > kSnapshotObjects) {
    return Observation();
  }
  aivision::object objects[kSnapshotObjects];
  for (int i = 0; i < count; ++i) {
    objects[i] = AIVision.objects[i];
  }
  return observeCup(objects, count);
}

void signal(AssistEvent event) {
  switch (event) {
    case AssistEvent::Locked: Controller.rumble("."); break;
    case AssistEvent::Arrived: Controller.rumble(".."); break;
    case AssistEvent::NotFound:
    case AssistEvent::Lost:
    case AssistEvent::TimedOut: Controller.rumble("---"); break;
    default: break;
  }
}

// Status on controller line 3 (the toggle display uses lines 1-2) so the driver can
// see whether a target was found, and why an approach ended. Each line is at most
// 20 characters, and it is redrawn only when the text changes.
void showStatus(AimTarget requested, const Observation& observation,
                const AssistOutput& output, int nowMs) {
  static char stickyText[24] = "";
  static char shown[24] = "";
  static int lastPrintMs = -1000;
  switch (output.event) {
    case AssistEvent::Arrived:
      snprintf(stickyText, sizeof(stickyText), "Arrived s%d", output.target.size);
      break;
    case AssistEvent::NotFound: snprintf(stickyText, sizeof(stickyText), "No target found"); break;
    case AssistEvent::Lost: snprintf(stickyText, sizeof(stickyText), "Lost target"); break;
    case AssistEvent::TimedOut: snprintf(stickyText, sizeof(stickyText), "Timed out"); break;
    case AssistEvent::Cancelled: snprintf(stickyText, sizeof(stickyText), "Cancelled by stick"); break;
    default: break;
  }
  if (output.event != AssistEvent::None && output.event != AssistEvent::Locked) {
    printf("Assist: %s (state %s)\n", stickyText, assist.stateName());
  }

  char text[24];
  if (requested != AimTarget::None) {
    const char* name = requested == AimTarget::Cup ? "Cup" : "Goal";
    if (observation.visible) {
      snprintf(text, sizeof(text), "%s ok e%+d s%d", name, observation.centerX - 160,
               observation.size);
    } else if (output.searching) {
      snprintf(text, sizeof(text), "%s searching...", name);
    } else {
      snprintf(text, sizeof(text), "%s NOT FOUND", name);
    }
  } else if (output.active && output.target.visible) {
    snprintf(text, sizeof(text), "Appr s%d e%+d", output.target.size,
             output.target.centerX - 160);
  } else if (output.active) {
    snprintf(text, sizeof(text), "Approaching...");
  } else {
    snprintf(text, sizeof(text), "%s", stickyText);
  }
  if (strcmp(text, shown) == 0 || nowMs - lastPrintMs < 120) {
    return;
  }
  lastPrintMs = nowMs;
  snprintf(shown, sizeof(shown), "%s", text);
  Controller.Screen.setCursor(3, 1);
  Controller.Screen.print("%-20s", text);
}

}

void assistReset() {
  assist.reset();
}

bool assistDrive() {
  static timer clock;
  const bool up = Controller.ButtonUp.pressing();
  const bool down = Controller.ButtonDown.pressing();
  if (up && down) {
    assist.reset();
    return false;
  }
  const AimTarget requested = up ? AimTarget::Cup : (down ? AimTarget::Goal : AimTarget::None);
  // Cancel on the same sticks the drive reads (Axis3 forward, Axis1 steering).
  const bool stickCancel = abs(Controller.Axis3.position()) >= kCancelStick ||
                           abs(Controller.Axis1.position()) >= kCancelStick;
  const AimTarget lookFor = assist.observationTarget(requested);
  const Observation observation = lookFor == AimTarget::None ? Observation() : observe(lookFor);
  const AssistOutput output = assist.update(requested, stickCancel, observation,
                                            static_cast<int>(clock.time(msec)));
  signal(output.event);
  showStatus(requested, observation, output, static_cast<int>(clock.time(msec)));
  if (!output.active) {
    return false;
  }
  driveInputs(output.forward, output.turn);
  return true;
}
