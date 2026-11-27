#pragma once

void updateAIVisionDisplay();

// Holds the sensor while snapshots and AIVision.objects are in use. A snapshot
// overwrites AIVision.objects, so the display and the driver assist take turns.
// Not recursive: do not call updateAIVisionDisplay() while holding it.
class AIVisionLock {
public:
  AIVisionLock();
  ~AIVisionLock();
  AIVisionLock(const AIVisionLock&) = delete;
  AIVisionLock& operator=(const AIVisionLock&) = delete;
};
