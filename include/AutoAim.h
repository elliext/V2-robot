#pragma once

#include "AprilTag.h"

// Include vex.h before this header.

// Driver assist: hold a button to turn toward the nearest target, release it to
// drive up to that target. The state machine has no hardware access so it can be
// tested on the host; AssistDrive.cpp connects it to the sensor and drivetrain.

enum class AimTarget { None, Cup, Goal };

struct Observation {
  bool visible = false;
  int centerX = 0;
  int size = 0;            // Apparent length in pixels; grows as the target nears.
  bool bottomClipped = false;
};

// Nearest upright cup, or an invisible observation.
Observation observeCup(const vex::aivision::object* objects, int count);
// Nearest goal, judged by the largest AprilTag.
Observation observeGoal(const GoalTagDetection* tags, int count);

enum class AssistEvent { None, Locked, Arrived, NotFound, Lost, TimedOut, Cancelled };

struct AssistOutput {
  bool active = false;     // True when the assist, not the sticks, drives this tick.
  double forward = 0;      // Arcade inputs, -100..100.
  double turn = 0;         // Positive turns clockwise.
  Observation target;      // What the assist acted on, including bridged dropouts.
  AssistEvent event = AssistEvent::None;
};

class AutoAim {
public:
  // requested is the button currently held. observation must describe
  // observationTarget(requested). stickCancel aborts an approach in progress.
  AssistOutput update(AimTarget requested, bool stickCancel,
                      const Observation& observation, int nowMs);
  // Which target the sensor should look for this tick; None means skip sampling.
  AimTarget observationTarget(AimTarget requested) const;
  const char* stateName() const;
  // Apparent size, in pixels, at which the approach counts as arrived.
  static int stopSize(AimTarget target);
  void reset();

private:
  enum class State { Idle, Aiming, Approaching };

  AssistOutput aim(const Observation& observation, int nowMs);
  AssistOutput approach(bool stickCancel, const Observation& observation, int nowMs);
  void trackSighting(const Observation& observation, int nowMs);
  Observation bridge(const Observation& observation, int nowMs) const;

  State state_ = State::Idle;
  Observation held_;
  AimTarget target_ = AimTarget::None;
  int startMs_ = 0;
  int lastSeenMs_ = 0;
  int lockedSinceMs_ = 0;
  bool locking_ = false;
  bool lockReported_ = false;
  bool notFoundReported_ = false;
  bool seen_ = false;
};
