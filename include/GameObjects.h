#pragma once

#include <string.h>

// Model class names. The sensor's model may report several pin variants, so a
// pin is any class whose name ends in "Pin" (for example "RedYellowPin").
// See knowledge/game-objects.md.

inline bool isCupClass(const char* className) {
  return strcmp(className, "Cup") == 0;
}

inline bool isPinClass(const char* className) {
  const size_t length = strlen(className);
  return length >= 3 && strcmp(className + length - 3, "Pin") == 0;
}
