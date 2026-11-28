#include "vex.h"
#include "AutoAim.h"

#include <iostream>

namespace fake {
int nowMs = 0;
}

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

vex::aivision::object cup(int centerX, int width, int height, int bottom) {
  vex::aivision::object object;
  object.type = vex::aivision::objectType::modelObject;
  object.exists = true;
  object.className = "Cup";
  object.centerX = centerX;
  object.originX = centerX - width / 2;
  object.width = width;
  object.height = height;
  object.originY = bottom - height;
  return object;
}

Observation seen(int centerX, int size, bool clipped = false) {
  Observation observation;
  observation.visible = true;
  observation.centerX = centerX;
  observation.size = size;
  observation.bottomClipped = clipped;
  return observation;
}

void selectsNearestUprightCup() {
  vex::aivision::object objects[5];
  objects[0] = cup(40, 40, 80, 150);   // Far upright cup.
  objects[1] = cup(250, 160, 80, 230); // Nearer, but tipped over.
  objects[2] = cup(200, 50, 100, 200); // Nearest upright cup.
  objects[3] = cup(100, 50, 100, 220);
  objects[3].className = "RedYellowPin";
  objects[4] = cup(120, 50, 100, 225);
  objects[4].exists = false;
  const Observation observation = observeCup(objects, 5);
  require(observation.visible && observation.centerX == 200 && observation.size == 100,
          "Cup choice must be the nearest upright cup");
  require(!observation.bottomClipped, "Cup well inside the image is not clipped");
  require(!observeCup(objects + 1, 1).visible, "A tipped cup must not be a target");
  vex::aivision::object angled = cup(100, 70, 85, 150);
  require(observeCup(&angled, 1).visible, "A cup seen from above, a little less than 2:1, still counts");
  require(observeCup(objects + 0, 1).visible && !observeCup(objects, 0).visible,
          "Empty input has no target");
  vex::aivision::object floorEdge = cup(100, 50, 100, 240);
  require(observeCup(&floorEdge, 1).bottomClipped, "Cup at the bottom edge is clipped");
}

void prefersCupHoldingPin() {
  vex::aivision::object objects[4];
  objects[0] = cup(60, 40, 80, 220);   // Nearest cup, empty.
  objects[1] = cup(200, 40, 80, 150);  // Farther cup holding a pin.
  objects[2] = cup(200, 30, 40, 100);  // Pin sticking out of cup 1.
  objects[2].className = "RedYellowPin";
  objects[3] = cup(260, 40, 80, 230);  // Even nearer cup, also empty.
  Observation observation = observeCup(objects, 4);
  require(observation.visible && observation.centerX == 200,
          "A cup holding a pin beats a nearer empty cup");

  objects[2] = cup(120, 30, 40, 100);  // Pin between the cups, in neither.
  objects[2].className = "RedYellowPin";
  observation = observeCup(objects, 4);
  require(observation.centerX == 260, "A pin outside every cup changes nothing");

  objects[2].exists = false;
  observation = observeCup(objects, 4);
  require(observation.centerX == 260, "With no pin the nearest cup is chosen");

  // Any pin class counts, not just the one used in testing.
  objects[2] = cup(200, 30, 40, 100);
  objects[2].className = "BlueOrangePin";
  require(observeCup(objects, 4).centerX == 200, "Any class ending in Pin counts as a pin");
  objects[2].className = "Pinwheel";
  require(observeCup(objects, 4).centerX == 260, "A class that only contains Pin is not a pin");
}

void selectsLargestGoalTag() {
  GoalTagDetection tags[2] = {{1, GoalRole::Neutral, 60, 100, 30, 28},
                              {0, GoalRole::Center, 250, 120, 40, 44}};
  const Observation observation = observeGoal(tags, 2);
  require(observation.visible && observation.centerX == 250 && observation.size == 44,
          "Goal choice must be the largest tag");
  require(!observeGoal(tags, 0).visible, "No tags means no goal");
}

void aimTurnsTowardTargetThenLocks() {
  AutoAim assist;
  AssistOutput output = assist.update(AimTarget::Cup, false, seen(260, 60), 0);
  require(output.active && output.turn > 0 && output.forward == 0,
          "Target on the right turns clockwise without driving");
  output = assist.update(AimTarget::Cup, false, seen(40, 60), 20);
  require(output.turn < 0, "Target on the left turns counterclockwise");
  output = assist.update(AimTarget::Cup, false, seen(161, 60), 40);
  require(output.active && output.turn == 0 && output.event == AssistEvent::None,
          "Centered target waits for a stable lock");
  output = assist.update(AimTarget::Cup, false, seen(158, 60), 200);
  require(output.event == AssistEvent::Locked, "Stable centered target reports lock");
  output = assist.update(AimTarget::Cup, false, seen(158, 60), 220);
  require(output.event == AssistEvent::None, "Lock is reported once");
}

void aimReportsMissingTarget() {
  AutoAim assist;
  AssistOutput output = assist.update(AimTarget::Goal, false, Observation(), 0);
  require(output.active && output.event == AssistEvent::None, "Short miss is silent");
  output = assist.update(AimTarget::Goal, false, Observation(), 600);
  require(output.event == AssistEvent::NotFound, "Persistent miss is reported");
  output = assist.update(AimTarget::None, false, Observation(), 700);
  require(!output.active && output.event == AssistEvent::None,
          "Release without a target returns the sticks and does not repeat the report");
  AutoAim tap;
  tap.update(AimTarget::Cup, false, Observation(), 0);
  output = tap.update(AimTarget::None, false, Observation(), 50);
  require(!output.active && output.event == AssistEvent::NotFound,
          "A tap that never saw a target is reported");
}

void releaseApproachesAndArrives() {
  AutoAim assist;
  assist.update(AimTarget::Cup, false, seen(150, 60), 0);
  require(assist.observationTarget(AimTarget::None) == AimTarget::Cup,
          "Release keeps looking for the aimed target");
  AssistOutput output = assist.update(AimTarget::None, false, seen(150, 60), 20);
  require(output.active && output.forward > 0, "Release starts driving forward");
  output = assist.update(AimTarget::None, false, seen(180, 60), 40);
  require(output.forward > 0 && output.turn > 0, "Approach steers back toward the target");
  const double farForward = output.forward;
  output = assist.update(AimTarget::None, false, seen(160, 110), 60);
  require(output.forward < farForward && output.forward > 0, "Approach slows when close");
  output = assist.update(AimTarget::None, false, seen(160, 120), 80);
  require(output.active && output.forward == 0 && output.event == AssistEvent::Arrived,
          "Reaching the stop size ends with a stop");
  require(assist.observationTarget(AimTarget::None) == AimTarget::None &&
              !assist.update(AimTarget::None, false, Observation(), 100).active,
          "Sticks resume after arrival");
}

void approachTurnsInPlaceForLargeError() {
  AutoAim assist;
  assist.update(AimTarget::Goal, false, seen(160, 30), 0);
  assist.update(AimTarget::None, false, seen(160, 30), 20);
  const AssistOutput output = assist.update(AimTarget::None, false, seen(10, 30), 40);
  require(output.forward == 0 && output.turn < 0, "A far off-center target turns before driving");
}

void approachEndsWhenBottomClipped() {
  AutoAim assist;
  assist.update(AimTarget::Cup, false, seen(160, 60), 0);
  assist.update(AimTarget::None, false, seen(160, 60), 20);
  const AssistOutput output = assist.update(AimTarget::None, false, seen(160, 90, true), 40);
  require(output.event == AssistEvent::Arrived, "A cup cut off at the image bottom is close enough");
}

void approachStopsWhenTargetLost() {
  AutoAim assist;
  assist.update(AimTarget::Cup, false, seen(160, 60), 0);
  assist.update(AimTarget::None, false, seen(160, 60), 20);
  AssistOutput output = assist.update(AimTarget::None, false, Observation(), 100);
  require(output.forward > 0 && output.event == AssistEvent::None,
          "A one-frame dropout keeps the last command");
  output = assist.update(AimTarget::None, false, Observation(), 250);
  require(output.active && output.forward == 0 && output.event == AssistEvent::None,
          "A longer dropout pauses");
  output = assist.update(AimTarget::None, false, Observation(), 400);
  require(output.event == AssistEvent::Lost && output.forward == 0, "A lost target stops");

  AutoAim near;
  near.update(AimTarget::Cup, false, seen(160, 60), 0);
  near.update(AimTarget::None, false, seen(160, 100), 20);
  output = near.update(AimTarget::None, false, Observation(), 400);
  require(output.event == AssistEvent::Arrived && output.forward == 0,
          "A target that vanishes while close counts as arrived");
}

void aimRidesThroughDropouts() {
  AutoAim assist;
  const AssistOutput first = assist.update(AimTarget::Cup, false, seen(260, 60), 0);
  require(first.turn > 0, "Target on the right turns clockwise");
  const AssistOutput dropped = assist.update(AimTarget::Cup, false, Observation(), 40);
  require(dropped.target.visible && dropped.turn == 0,
          "A dropout keeps the target shown but stops turning");
  const AssistOutput back = assist.update(AimTarget::Cup, false, seen(260, 60), 60);
  require(back.turn > 0, "Turning resumes when the target reappears");
  const AssistOutput gone = assist.update(AimTarget::Cup, false, Observation(), 400);
  require(!gone.target.visible && gone.turn == 0, "Aim shows no target once the hold expires");
}

void approachIsCancelledAndTimesOut() {
  AutoAim cancelled;
  cancelled.update(AimTarget::Cup, false, seen(160, 60), 0);
  cancelled.update(AimTarget::None, false, seen(160, 60), 20);
  AssistOutput output = cancelled.update(AimTarget::None, true, seen(160, 60), 40);
  require(output.event == AssistEvent::Cancelled && output.forward == 0, "Sticks cancel");
  require(!cancelled.update(AimTarget::None, true, seen(160, 60), 60).active,
          "Sticks drive after cancelling");

  AutoAim slow;
  slow.update(AimTarget::Cup, false, seen(160, 60), 0);
  slow.update(AimTarget::None, false, seen(160, 60), 20);
  output = slow.update(AimTarget::None, false, seen(160, 60), 5100);
  require(output.event == AssistEvent::TimedOut, "A stalled approach times out");
}

void stickCancelDoesNotAbortAiming() {
  AutoAim assist;
  const AssistOutput output = assist.update(AimTarget::Cup, true, seen(260, 60), 0);
  require(output.active && output.turn > 0, "Held button keeps aiming despite stick input");
}

void searchSweepsSlightlyAndStopsOnSighting() {
  AutoAim assist;
  AssistOutput output = assist.update(AimTarget::Cup, false, Observation(), 0);
  require(output.active && output.turn == 0 && !output.searching, "Search waits before starting");
  double net = 0;
  double maxTurn = 0;
  bool sawLeft = false, sawRight = false, sawPause = false;
  for (int t = 20; t <= 3000; t += 20) {
    output = assist.update(AimTarget::Cup, false, Observation(), t);
    net += output.turn;
    maxTurn = output.turn > maxTurn ? output.turn : (-output.turn > maxTurn ? -output.turn : maxTurn);
    sawRight = sawRight || output.turn > 0;
    sawLeft = sawLeft || output.turn < 0;
    sawPause = sawPause || (t > 500 && output.searching && output.turn == 0);
  }
  require(output.searching, "Search continues while nothing is seen");
  require(sawLeft && sawRight, "Search sweeps both ways");
  require(sawPause, "Search pauses between pulses so the sensor sees a steady frame");
  require(maxTurn <= 25, "Search turns gently");
  require(net > -25 * 50 && net < 25 * 50, "Search stays near the starting heading");

  output = assist.update(AimTarget::Cup, false, seen(160, 60), 3020);
  require(!output.searching && output.turn == 0, "Search stops the moment a target is seen");
}

void searchStartsTowardLastSeenSide() {
  AutoAim assist;
  assist.update(AimTarget::Cup, false, seen(20, 60), 0);
  AssistOutput output;
  for (int t = 20; t < 600; t += 20) {
    output = assist.update(AimTarget::Cup, false, Observation(), t);
    if (output.turn != 0) {
      break;
    }
  }
  require(output.turn < 0, "Search begins toward the side the target was last seen");
}

void newPressRestartsWithNewTarget() {
  AutoAim assist;
  assist.update(AimTarget::Cup, false, seen(160, 60), 0);
  assist.update(AimTarget::None, false, seen(160, 60), 20);
  const AssistOutput output = assist.update(AimTarget::Goal, false, seen(260, 30), 40);
  require(output.forward == 0 && output.turn > 0 &&
              assist.observationTarget(AimTarget::None) == AimTarget::Goal,
          "Pressing again re-aims at the newly requested target");
}
}

int main() {
  try {
    selectsNearestUprightCup();
    prefersCupHoldingPin();
    selectsLargestGoalTag();
    aimTurnsTowardTargetThenLocks();
    aimReportsMissingTarget();
    releaseApproachesAndArrives();
    approachTurnsInPlaceForLargeError();
    approachEndsWhenBottomClipped();
    approachStopsWhenTargetLost();
    aimRidesThroughDropouts();
    approachIsCancelledAndTimesOut();
    stickCancelDoesNotAbortAiming();
    newPressRestartsWithNewTarget();
    searchSweepsSlightlyAndStopsOnSighting();
    searchStartsTowardLastSeenSide();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << "\n";
    return 1;
  }
  std::cout << "AutoAim tests passed\n";
  return 0;
}
