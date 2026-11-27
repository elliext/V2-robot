#include "vex.h"
#include "AprilTag.h"

using namespace vex;

namespace {
constexpr int kSnapshotObjects = 24;
}

bool classifyGoalTag(int tagId, GoalRole& role) {
  if (tagId == 0) {
    role = GoalRole::Center;
    return true;
  }
  // Blue may reuse 1-4 or use 5-8 (unconfirmed), so the position within the
  // group of four is tagId % 4; remainder 0 is tag 4.
  if (tagId < 1 || tagId > 8) {
    return false;
  }
  switch (tagId % 4) {
    case 2:
    case 3: role = GoalRole::Alliance; return true;
    default: role = GoalRole::Neutral; return true;
  }
}

int detectGoalTags(GoalTagDetection* out, int maxOut) {
  const int count = AIVision.takeSnapshot(aivision::ALL_TAGS, kSnapshotObjects);
  if (count < 0 || count > kSnapshotObjects) {
    return -1;
  }
  int written = 0;
  for (int i = 0; i < count && written < maxOut; ++i) {
    const auto& object = AIVision.objects[i];
    GoalRole role;
    if (!object.exists || object.type != aivision::objectType::tagObject ||
        !classifyGoalTag(static_cast<int>(object.id), role)) {
      continue;
    }
    out[written++] = {static_cast<int>(object.id), role, object.centerX,
                      object.centerY, object.width, object.height};
  }
  return written;
}
