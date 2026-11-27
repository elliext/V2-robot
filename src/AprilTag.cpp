#include "vex.h"
#include "AprilTag.h"

using namespace vex;

namespace {
constexpr int kSnapshotObjects = 24;
}

bool classifyGoalTag(int tagId, GoalTag& goal) {
  if (tagId == 0) {
    goal = {GoalRole::Center, 0};
    return true;
  }
  // Blue may reuse 1-4 or use 5-8 (unconfirmed), so the position within the
  // group of four is tagId % 4; remainder 0 is tag 4.
  if (tagId < 1 || tagId > 8) {
    return false;
  }
  switch (tagId % 4) {
    case 1: goal = {GoalRole::Neutral, 1}; return true;
    case 2: goal = {GoalRole::Alliance, 1}; return true;
    case 3: goal = {GoalRole::Alliance, 2}; return true;
    default: goal = {GoalRole::Neutral, 2}; return true;
  }
}

const char* goalQuadrantName(const GoalTag& goal, bool redAlliance) {
  if (goal.role == GoalRole::Center) {
    return "Center";
  }
  if (goal.quadrantNumber == 1) {
    return redAlliance ? "Red 1" : "Blue 1";
  }
  return redAlliance ? "Red 2" : "Blue 2";
}

int detectGoalTags(GoalTagDetection* out, int maxOut) {
  const int count = AIVision.takeSnapshot(aivision::ALL_TAGS, kSnapshotObjects);
  if (count < 0 || count > kSnapshotObjects) {
    return -1;
  }
  int written = 0;
  for (int i = 0; i < count && written < maxOut; ++i) {
    const auto& object = AIVision.objects[i];
    GoalTag goal;
    if (!object.exists || object.type != aivision::objectType::tagObject ||
        !classifyGoalTag(static_cast<int>(object.id), goal)) {
      continue;
    }
    out[written++] = {static_cast<int>(object.id), goal, object.centerX,
                      object.centerY, object.width, object.height};
  }
  return written;
}
