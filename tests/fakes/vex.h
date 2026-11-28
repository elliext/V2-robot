#pragma once

#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <math.h>

namespace fake {
extern int nowMs;
}

namespace vex {
enum timeUnits { msec };
enum directionType { forward, reverse };
enum velocityUnits { rpm, percent };
enum rotationUnits { degrees, deg = degrees };
enum class color { black, white };
enum brakeType { hold, brake, coast };
enum class ledState { on, off };

void wait(int duration, timeUnits);

class motor {
public:
  bool connected = true;
  double positionDegrees = 0;
  bool lastSpinWaitForCompletion = true;
  bool installed() const { return connected; }
  double position(rotationUnits) const { return positionDegrees; }
  void setPosition(double value, rotationUnits) { positionDegrees = value; }
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
  void spinFor(directionType direction, double rotation, rotationUnits,
               double speed, velocityUnits, bool waitForCompletion = true) {
    lastSpinWaitForCompletion = waitForCompletion;
    commands.push_back({"spinForRotation",
                        direction == forward ? rotation : -rotation, speed});
  }
  void spin(directionType direction, double speed, velocityUnits) {
    commands.push_back({"spin", direction == forward ? speed : -speed, 0});
  }
  void stop(brakeType mode) {
    commands.push_back({"stop", static_cast<double>(mode), 0});
  }
};

class motor_group : public motor {};
class mutex {
public:
  void lock() {}
  void unlock() {}
};
class optical {
public:
  bool connected = true;
  double sampleHue = 0;
  bool nearby = false;
  bool installed() const { return connected; }
  double hue() const { return sampleHue; }
  bool isNearObject() const { return nearby; }
  void objectDetectThreshold(int) {}
  void setLightPower(double, velocityUnits) {}
  void setLight(ledState) {}
};
class inertial {
public:
  bool connected = true;
  bool calibrating = false;
  double headingDegrees = 0;
  double rotationDegrees = 0;
  int rotationResetCalls = 0;
  bool installed() const { return connected; }
  bool isCalibrating() const { return calibrating; }
  double heading(rotationUnits) const { return headingDegrees; }
  double rotation() const { return rotationDegrees; }
  void setRotation(double value, rotationUnits) {
    rotationDegrees = value;
    ++rotationResetCalls;
  }
};
class competition {
public:
  bool enabled = false;
  bool autonomous = false;
  bool isEnabled() const { return enabled; }
  bool isAutonomous() const { return autonomous; }
  bool isDriverControl() const { return false; }
};

class controller {
public:
  struct button {
    bool pressed = false;
    bool pressing() const { return pressed; }
  };
  button ButtonA, ButtonX, ButtonLeft, ButtonRight, ButtonUp, ButtonDown, ButtonL1, ButtonR1;
  struct screen {
    std::string text;
    int clearCalls = 0;
    int printCalls = 0;
    int cursorRow = 3;
    std::array<std::string, 3> lines;
    void clearLine(int line) {
      if (line < 1 || line > 3) {
        throw std::runtime_error("Invalid controller line");
      }
      text.clear();
      lines.at(line - 1).clear();
      ++clearCalls;
    }
    void setCursor(int row, int column) {
      if (row < 1 || row > 3 || column != 1) {
        throw std::runtime_error("Unexpected controller cursor");
      }
      cursorRow = row;
    }
    void print(const char* format, ...) {
      ++printCalls;
      char buffer[256];
      va_list arguments;
      va_start(arguments, format);
      const int length = std::vsnprintf(buffer, sizeof(buffer), format, arguments);
      va_end(arguments);
      if (length < 0 || length >= static_cast<int>(sizeof(buffer))) {
        throw std::runtime_error("Invalid controller output");
      }
      text = buffer;
      lines.at(cursorRow - 1) = text;
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
    bool touchDown = false;
    int touchX = 0;
    int touchY = 0;

    bool pressing() const { return touchDown; }
    int xPosition() const { return touchX; }
    int yPosition() const { return touchY; }
    void drawRectangle(int, int, int, int) {}
    void setPenColor(color) {}
    void setFillColor(color) {}

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
    std::int16_t originX = 0;
    std::int16_t originY = 0;
    std::int16_t width = 0;
    std::int16_t height = 0;
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
  static const aiobjdesc ALL_TAGS;

  // A tag snapshot overwrites objects, as on the real sensor; the next model
  // snapshot restores what the test had placed there.
  std::array<object, 24> tagData;
  std::array<object, 24> modelData;
  bool modelSaved = false;
  int tagSnapshotCalls = 0;
  int tagSnapshotResult = 0;

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
    if (&description == &ALL_TAGS) {
      ++tagSnapshotCalls;
      if (tagSnapshotResult > 0) {
        if (!modelSaved) {
          modelData = objects.data;
          modelSaved = true;
        }
        objects.data = tagData;
      }
      return tagSnapshotResult;
    }
    if (&description != &ALL_AIOBJS) {
      throw std::runtime_error("Snapshot did not request AI model objects");
    }
    if (modelSaved) {
      objects.data = modelData;
      modelSaved = false;
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
extern vex::motor toggleLeft;
extern vex::motor toggleRight;
extern vex::optical toggleOptical1;
extern vex::optical toggleOptical2;
