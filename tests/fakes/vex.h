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
enum directionType { forward, reverse };
enum velocityUnits { rpm, percent };
enum brakeType { hold, brake };

void wait(int duration, timeUnits);

class motor {
public:
  struct command {
    std::string kind;
    double value;
    double speed;
  };
  std::vector<command> commands;
  void spinFor(directionType direction, double duration, timeUnits,
               double speed, velocityUnits) {
    commands.push_back({"spinFor", direction == forward ? duration : -duration, speed});
  }
  void spin(directionType direction, double speed, velocityUnits) {
    commands.push_back({"spin", direction == forward ? speed : -speed, 0});
  }
  void stop(brakeType mode) {
    commands.push_back({"stop", static_cast<double>(mode), 0});
  }
};

class motor_group : public motor {};
class optical {};
class inertial {};
class competition {};

class controller {
public:
  struct button {
    bool pressed = false;
    bool pressing() const { return pressed; }
  };
  button ButtonA, ButtonLeft, ButtonRight, ButtonUp, ButtonDown, ButtonL1, ButtonR1;
  struct screen {
    std::string text;
    void clearLine(int line) {
      if (line != 3) {
        throw std::runtime_error("Selector must use controller line 3");
      }
      text.clear();
    }
    void setCursor(int row, int column) {
      if (row != 3 || column != 1) {
        throw std::runtime_error("Unexpected selector cursor");
      }
    }
    void print(const char* format, ...) {
      char buffer[256];
      va_list arguments;
      va_start(arguments, format);
      const int length = std::vsnprintf(buffer, sizeof(buffer), format, arguments);
      va_end(arguments);
      if (length < 0 || length >= static_cast<int>(sizeof(buffer))) {
        throw std::runtime_error("Invalid controller output");
      }
      text = buffer;
      if (text.size() > 20) {
        throw std::runtime_error("Selector exceeds controller line width");
      }
    }
  } Screen;
  int rumbleCalls = 0;
  void rumble(const char*) { ++rumbleCalls; }
};

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
extern vex::controller Controller;
