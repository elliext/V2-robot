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
RawObject rawObjects[3];
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
  for (int i = 0; i < count && rawShown < 3; ++i) {
    RawObject& raw = rawObjects[rawShown++];
    snprintf(raw.name, sizeof(raw.name), "%s", objects[i].className);
    raw.width = objects[i].width;
    raw.height = objects[i].height;
    raw.accepted = observeCup(&objects[i], 1).visible;
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
  if (requested != AimTarget::None && previousRequest == AimTarget::None) {
    peakSize = lastSize = 0;
  }
  if (requested != AimTarget::None) {
    shownTarget = requested;
  }
  previousRequest = requested;
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
  Brain.Screen.printAt(10, 20, true, "AIM TEST: no motor output");
  Brain.Screen.printAt(10, 42, true, "Hold Up=cup, Down=goal; release=approach");
  Brain.Screen.printAt(10, 66, true, "Sensor: %s", AIVision.installed() ? "ok" : "MISSING (port 6)");
  Brain.Screen.printAt(10, 90, true, "Held: %s   State: %s",
                       requested == AimTarget::Cup ? "Up/cup" :
                       (requested == AimTarget::Goal ? "Down/goal" : "none"),
                       assist.stateName());
  if (output.target.visible) {
    Brain.Screen.printAt(10, 114, true, "Target x=%d size=%d  error=%d px", output.target.centerX,
                         output.target.size, output.target.centerX - 160);
  } else {
    Brain.Screen.printAt(10, 114, true, "Target: none");
  }
  Brain.Screen.printAt(10, 138, true, "Would turn %d  forward %d",
                       static_cast<int>(output.turn), static_cast<int>(output.forward));
  Brain.Screen.printAt(10, 162, true, "Last event: %s (%d ms ago)", eventName(lastEvent),
                       lastEvent == AssistEvent::None ? 0 : nowMs - lastEventMs);
  Brain.Screen.printAt(10, 186, true, "Size: last seen %d  peak %d  stop at %d", lastSize, peakSize,
                       shownTarget == AimTarget::None ? 0 : AutoAim::stopSize(shownTarget));
  if (shownTarget == AimTarget::Cup) {
    Brain.Screen.printAt(10, 210, true, "Raw: %d objects (upright cup = h>=w)", rawTotal);
    for (int i = 0; i < rawShown; ++i) {
      Brain.Screen.printAt(10, 234 + i * 22, true, "%.14s %dx%d %s", rawObjects[i].name,
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
