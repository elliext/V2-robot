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

#if defined(AIM_TEST)
// Raw model objects from the latest cup snapshot, shown to tune the cup filter.
struct RawObject {
  char name[16];
  int width;
  int height;
  bool accepted;
};
constexpr int kRawRows = 4;
RawObject rawObjects[kRawRows];
int rawShown = 0;
int rawTotal = -1;
#endif

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
#if defined(AIM_TEST)
  rawTotal = count > 0 ? count : 0;
  rawShown = 0;
#endif
  if (count <= 0 || count > kSnapshotObjects) {
    return Observation();
  }
  aivision::object objects[kSnapshotObjects];
  for (int i = 0; i < count; ++i) {
    objects[i] = AIVision.objects[i];
  }
#if defined(AIM_TEST)
  // Accepted objects first so a rejected pin cannot push the cup off the screen.
  for (int pass = 0; pass < 2; ++pass) {
    for (int i = 0; i < count && rawShown < kRawRows; ++i) {
      const bool accepted = observeCup(&objects[i], 1).visible;
      if (accepted != (pass == 0)) {
        continue;
      }
      RawObject& raw = rawObjects[rawShown++];
      snprintf(raw.name, sizeof(raw.name), "%s", objects[i].className);
      raw.width = objects[i].width;
      raw.height = objects[i].height;
      raw.accepted = accepted;
    }
  }
#endif
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

#if defined(AIM_TEST)
const char* eventName(AssistEvent event) {
  switch (event) {
    case AssistEvent::Locked: return "Locked";
    case AssistEvent::Arrived: return "Arrived";
    case AssistEvent::NotFound: return "Not found";
    case AssistEvent::Lost: return "Lost";
    case AssistEvent::TimedOut: return "Timed out";
    case AssistEvent::Cancelled: return "Cancelled";
    default: return "none";
  }
}

// The display normally configures the sensor; the test build has no display task.
void configureSensor() {
  static bool configured = false;
  if (!AIVision.installed()) {
    configured = false;
    return;
  }
  if (!configured) {
    AIVisionLock sensorLock;
    AIVision.colorDetection(false);
    AIVision.tagDetection(true);
    AIVision.modelDetection(true);
    configured = true;
  }
}

void showAimTest(AimTarget requested, const AssistOutput& output, int nowMs) {
  static AssistEvent lastEvent = AssistEvent::None;
  static int lastEventMs = 0;
  static timer refresh;
  static AimTarget previousRequest = AimTarget::None;
  static AimTarget shownTarget = AimTarget::None;
  static int peakSize = 0;
  static int lastSize = 0;
  static int peakForward = 0;
  if (requested != AimTarget::None && previousRequest == AimTarget::None) {
    peakSize = lastSize = peakForward = 0;
  }
  if (requested != AimTarget::None) {
    shownTarget = requested;
  }
  previousRequest = requested;
  const int forwardNow = static_cast<int>(output.forward);
  peakForward = forwardNow > peakForward ? forwardNow : peakForward;
  if (output.target.visible) {
    lastSize = output.target.size;
    peakSize = lastSize > peakSize ? lastSize : peakSize;
  }
  if (output.event != AssistEvent::None) {
    lastEvent = output.event;
    lastEventMs = nowMs;
  }
  if (refresh.time(msec) < 50) {
    return;
  }
  refresh.reset();
  Brain.Screen.clearScreen();
  // The screen is 240 px tall: keep every row at or above y=230.
  // A changing counter tells a frozen program from a quiet scene.
  Brain.Screen.printAt(10, 16, true, "AIM TEST (no motors)  up %d s", nowMs / 1000);
  Brain.Screen.printAt(10, 36, true, "Sensor %s  Held %s  %s",
                       AIVision.installed() ? "ok" : "MISSING",
                       requested == AimTarget::Cup ? "Up" :
                       (requested == AimTarget::Goal ? "Down" : "-"),
                       assist.stateName());
  if (output.target.visible) {
    Brain.Screen.printAt(10, 56, true, "Target x=%d size=%d err=%d", output.target.centerX,
                         output.target.size, output.target.centerX - 160);
  } else {
    Brain.Screen.printAt(10, 56, true, "Target: none");
  }
  Brain.Screen.printAt(10, 76, true, "Turn %d  Fwd %d  (peak fwd %d)",
                       static_cast<int>(output.turn), forwardNow, peakForward);
  Brain.Screen.printAt(10, 96, true, "Event: %s (%d ms ago)", eventName(lastEvent),
                       lastEvent == AssistEvent::None ? 0 : nowMs - lastEventMs);
  Brain.Screen.printAt(10, 116, true, "Size last %d  peak %d  stop %d", lastSize, peakSize,
                       shownTarget == AimTarget::None ? 0 : AutoAim::stopSize(shownTarget));
  if (shownTarget == AimTarget::Cup) {
    Brain.Screen.printAt(10, 136, true, "Raw: %d objects", rawTotal);
    for (int i = 0; i < rawShown; ++i) {
      Brain.Screen.printAt(10, 156 + i * 20, true, "%.12s %dx%d %s", rawObjects[i].name,
                           rawObjects[i].width, rawObjects[i].height,
                           rawObjects[i].accepted ? "ACCEPT" : "reject");
    }
  }
  Brain.Screen.render();
}
#endif
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
#if defined(AIM_TEST)
  configureSensor();
#endif
  const Observation observation = lookFor == AimTarget::None ? Observation() : observe(lookFor);
  const AssistOutput output = assist.update(requested, stickCancel, observation,
                                            static_cast<int>(clock.time(msec)));
  signal(output.event);
  showStatus(requested, observation, output, static_cast<int>(clock.time(msec)));
#if defined(AIM_TEST)
  // Motor-free test: show what the assist would command, never drive.
  showAimTest(requested, output, static_cast<int>(clock.time(msec)));
  return output.active;
#else
  if (!output.active) {
    return false;
  }
  driveInputs(output.forward, output.turn);
  return true;
#endif
}
