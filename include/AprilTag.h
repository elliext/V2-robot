#pragma once

// AprilTag goal identification. Field facts live in knowledge/field-setup.md.
// Tags 1-4 repeat on both alliances, so a tag alone gives the goal's role and
// quadrant number (1 or 2) but never its alliance color.

enum class GoalRole { Center, Alliance, Neutral };

struct GoalTag {
  GoalRole role;
  // 0 for the center goal; otherwise 1 (Red 1/Blue 1) or 2 (Red 2/Blue 2).
  int quadrantNumber;
};

struct GoalTagDetection {
  int tagId;
  GoalTag goal;
  int centerX;
  int centerY;
  int width;
  int height;
};

// Tag 0 is the center goal. Otherwise the tag's position in its group of four
// (tagId % 4, with 0 meaning 4) selects the goal: 2 and 3 are alliance goals,
// 1 and 4 are neutral. IDs 1-8 are accepted because it is unconfirmed whether
// Blue uses 1-4 or 5-8. Returns false for IDs that are not goal tags.
bool classifyGoalTag(int tagId, GoalTag& goal);

// "Center", "Red 1", "Blue 2", ... The caller supplies the alliance because
// the tag cannot reveal it.
const char* goalQuadrantName(const GoalTag& goal, bool redAlliance);

// Snapshots AprilTags and writes up to maxOut recognised goal tags to out.
// Returns the number written, or -1 if the sensor reported an invalid count.
// Overwrites AIVision.objects, so read any model objects first.
int detectGoalTags(GoalTagDetection* out, int maxOut);
