#pragma once

#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace fake {
extern int nowMs;
}

namespace vex {
enum timeUnits { msec };

class timer {
  int startMs = fake::nowMs;
public:
  double time(timeUnits) const { return fake::nowMs - startMs; }
  void reset() { startMs = fake::nowMs; }
};

class brain {
public:
  struct screen {
    struct line {
      int y;
      std::string text;
    };
    int clearCalls = 0;
    std::vector<line> lines;

    void clearScreen() {
      ++clearCalls;
      lines.clear();
    }

    void printAt(int, int y, bool, const char* format, ...) {
      char text[256];
      va_list arguments;
      va_start(arguments, format);
      const int length = std::vsnprintf(text, sizeof(text), format, arguments);
      va_end(arguments);
      if (length < 0 || length >= static_cast<int>(sizeof(text))) {
        throw std::runtime_error("Invalid or truncated screen output");
      }
      lines.push_back({y, text});
    }
  } Screen;
};

class aivision {
public:
  enum class objectType {
    unknownObject = 0,
    colorObject = 1,
    codeObject = 2,
    modelObject = 4,
    tagObject = 8
  };

  struct object {
    std::int32_t id = 0;
    objectType type = objectType::unknownObject;
    bool exists = false;
    const char* className = "";
    std::int16_t score = 0;
    std::int16_t centerX = 0;
    std::int16_t centerY = 0;
  };

  struct objectArray {
    std::array<object, 24> data;
    int reads = 0;
    object& operator[](std::size_t index) {
      ++reads;
      return data.at(index);
    }
  } objects;

  struct aiobjdesc {};
  static const aiobjdesc ALL_AIOBJS;

  bool connected = true;
  bool colorsEnabled = true;
  bool tagsEnabled = true;
  bool modelEnabled = false;
  int configurationCalls = 0;
  int snapshotCalls = 0;
  int requestedObjects = 0;
  int snapshotResult = 0;

  bool installed() const { return connected; }
  void colorDetection(bool enabled) { colorsEnabled = enabled; }
  void tagDetection(bool enabled) { tagsEnabled = enabled; }
  void modelDetection(bool enabled) {
    modelEnabled = enabled;
    ++configurationCalls;
  }
  int takeSnapshot(const aiobjdesc& description, int count) {
    if (&description != &ALL_AIOBJS) {
      throw std::runtime_error("Snapshot did not request AI model objects");
    }
    ++snapshotCalls;
    requestedObjects = count;
    return snapshotResult;
  }
};
}

extern vex::brain Brain;
extern vex::aivision AIVision;
