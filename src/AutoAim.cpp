#include "vex.h"
#include "AutoAim.h"
#include "GameObjects.h"

namespace {
// Pixel values are for the 320x240 AI Vision image. The stop sizes and the aim
// point are estimates: tune them on the robot (a printf reports the final size).
constexpr int kImageHeight = 240;
constexpr int kAimCenterX = 160;       // Shift to line the claw, not the camera, up.
constexpr int kLockTolerancePx = 8;
constexpr int kLockStableMs = 150;
constexpr double kAimKp = 0.3;
constexpr double kMinAimTurn = 14;     // Below this the drivetrain stalls.
constexpr double kMaxAimTurn = 35;
constexpr int kNotFoundMs = 500;
constexpr int kLostMs = 300;
constexpr int kDropoutHoldMs = 200;   // Reuse the last sighting through brief dropouts.
constexpr int kApproachTimeoutMs = 5000;
constexpr double kApproachKp = 0.25;
constexpr double kMaxApproachTurn = 20;
constexpr int kApproachMaxErrorPx = 60; // Beyond this, turn in place before driving.
constexpr double kMinForward = 15;
constexpr double kMaxForward = 45;
constexpr int kCupStopSize = 120;      // Measured: the cup is lost around 132-141 px.
constexpr double kNearFraction = 0.8;  // Lost beyond this share of the stop size counts as arrived.
constexpr int kGoalStopSize = 80;
// Upright cups are about 2:1 tall, tipped ones about 1:2; 1 splits them evenly
// and tolerates a cup seen from above or at an angle.
constexpr double kUprightRatio = 1.0;

double clamp(double value, double low, double high) {
  return value < low ? low : (value > high ? high : value);
}

double aimTurn(int error) {
  if (error > -kLockTolerancePx && error < kLockTolerancePx) {
    return 0;
  }
  const double magnitude = clamp(kAimKp * (error < 0 ? -error : error),
                                 kMinAimTurn, kMaxAimTurn);
  return error < 0 ? -magnitude : magnitude;
}

AssistOutput finished(AssistEvent event) {
  AssistOutput output;
  output.active = true;
  output.event = event;
  return output;
}
}

namespace {
// A pin counts as in the cup when its center is within the cup's width and the
// two boxes overlap vertically; a pin standing in a cup usually sticks out the top.
bool pinInCup(const vex::aivision::object& cup, const vex::aivision::object* objects,
              int count) {
  for (int i = 0; i < count; ++i) {
    const auto& pin = objects[i];
    if (!pin.exists || pin.type != vex::aivision::objectType::modelObject ||
        !isPinClass(pin.className)) {
      continue;
    }
    const bool withinWidth = pin.centerX >= cup.originX &&
                             pin.centerX <= cup.originX + cup.width;
    const bool overlapsVertically = pin.originY < cup.originY + cup.height &&
                                    pin.originY + pin.height > cup.originY;
    if (withinWidth && overlapsVertically) {
      return true;
    }
  }
  return false;
}
}

Observation observeCup(const vex::aivision::object* objects, int count) {
  Observation best;
  int bestBottom = 0;
  bool bestHasPin = false;
  for (int i = 0; i < count; ++i) {
    const auto& object = objects[i];
    if (!object.exists || object.type != vex::aivision::objectType::modelObject ||
        !isCupClass(object.className) || object.width <= 0 ||
        object.height < kUprightRatio * object.width) {
      continue;
    }
    // Prefer a cup that already holds a pin; among equals, the nearest. On the
    // floor, a nearer cup sits lower in the image.
    const bool hasPin = pinInCup(object, objects, count);
    const int bottom = object.originY + object.height;
    const bool better = !best.visible || (hasPin && !bestHasPin) ||
                        (hasPin == bestHasPin && bottom > bestBottom);
    if (better) {
      best.visible = true;
      best.centerX = object.centerX;
      best.size = object.height;
      best.bottomClipped = bottom >= kImageHeight - 2;
      bestBottom = bottom;
      bestHasPin = hasPin;
    }
  }
  return best;
}

Observation observeGoal(const GoalTagDetection* tags, int count) {
  Observation best;
  for (int i = 0; i < count; ++i) {
    const int size = tags[i].width > tags[i].height ? tags[i].width : tags[i].height;
    if (size > 0 && (!best.visible || size > best.size)) {
      best.visible = true;
      best.centerX = tags[i].centerX;
      best.size = size;
      best.bottomClipped = tags[i].centerY + tags[i].height / 2 >= kImageHeight - 2;
    }
  }
  return best;
}

void AutoAim::reset() {
  *this = AutoAim();
}

AimTarget AutoAim::observationTarget(AimTarget requested) const {
  if (requested != AimTarget::None) {
    return requested;
  }
  return state_ == State::Idle ? AimTarget::None : target_;
}

int AutoAim::stopSize(AimTarget target) {
  return target == AimTarget::Cup ? kCupStopSize : kGoalStopSize;
}

const char* AutoAim::stateName() const {
  return state_ == State::Aiming ? "Aiming" :
         (state_ == State::Approaching ? "Approaching" : "Idle");
}

void AutoAim::trackSighting(const Observation& observation, int nowMs) {
  if (observation.visible) {
    seen_ = true;
    lastSeenMs_ = nowMs;
    held_ = observation;
  }
}

// The model drops a detection for a frame or two now and then. Reusing the last
// sighting briefly keeps the aim steady; call after trackSighting.
Observation AutoAim::bridge(const Observation& observation, int nowMs) const {
  if (!observation.visible && seen_ && nowMs - lastSeenMs_ <= kDropoutHoldMs) {
    return held_;
  }
  return observation;
}

AssistOutput AutoAim::update(AimTarget requested, bool stickCancel,
                             const Observation& observation, int nowMs) {
  if (requested != AimTarget::None) {
    if (state_ != State::Aiming || target_ != requested) {
      reset();
      state_ = State::Aiming;
      target_ = requested;
      startMs_ = nowMs;
    }
    return aim(observation, nowMs);
  }
  if (state_ == State::Aiming) {
    // Button released: approach only if the target was seen very recently.
    if (seen_ && nowMs - lastSeenMs_ <= kLostMs) {
      state_ = State::Approaching;
      startMs_ = nowMs;
      return approach(false, observation, nowMs);
    }
    const bool reported = notFoundReported_;
    state_ = State::Idle;
    AssistOutput output;
    output.event = reported ? AssistEvent::None : AssistEvent::NotFound;
    return output;
  }
  if (state_ == State::Approaching) {
    return approach(stickCancel, observation, nowMs);
  }
  return AssistOutput();
}

AssistOutput AutoAim::aim(const Observation& raw, int nowMs) {
  AssistOutput output;
  output.active = true;
  trackSighting(raw, nowMs);
  const Observation observation = bridge(raw, nowMs);
  output.target = observation;
  if (!observation.visible) {
    locking_ = false;
    const int since = seen_ ? lastSeenMs_ : startMs_;
    if (!notFoundReported_ && nowMs - since >= kNotFoundMs) {
      notFoundReported_ = true;
      output.event = AssistEvent::NotFound;
    }
    return output;
  }
  notFoundReported_ = false;
  const int error = observation.centerX - kAimCenterX;
  output.turn = aimTurn(error);
  if (output.turn != 0) {
    locking_ = false;
    return output;
  }
  if (!locking_) {
    locking_ = true;
    lockedSinceMs_ = nowMs;
  }
  if (!lockReported_ && nowMs - lockedSinceMs_ >= kLockStableMs) {
    lockReported_ = true;
    output.event = AssistEvent::Locked;
  }
  return output;
}

AssistOutput AutoAim::approach(bool stickCancel, const Observation& raw, int nowMs) {
  if (stickCancel) {
    state_ = State::Idle;
    return finished(AssistEvent::Cancelled);
  }
  if (nowMs - startMs_ >= kApproachTimeoutMs) {
    state_ = State::Idle;
    return finished(AssistEvent::TimedOut);
  }
  trackSighting(raw, nowMs);
  const Observation observation = bridge(raw, nowMs);
  AssistOutput output;
  output.active = true;
  output.target = observation;
  const int stopSize = AutoAim::stopSize(target_);
  if (!observation.visible) {
    if (nowMs - lastSeenMs_ > kLostMs) {
      state_ = State::Idle;
      // A target that vanished while already close is too near to see, not lost.
      return finished(held_.size >= kNearFraction * stopSize ? AssistEvent::Arrived
                                                             : AssistEvent::Lost);
    }
    return output;
  }
  if (observation.size >= stopSize || observation.bottomClipped) {
    printf("Assist: arrived; target size %d px (stop at %d).\n", observation.size,
           stopSize);
    state_ = State::Idle;
    return finished(AssistEvent::Arrived);
  }
  const int error = observation.centerX - kAimCenterX;
  if (error > kApproachMaxErrorPx || error < -kApproachMaxErrorPx) {
    output.turn = aimTurn(error);
    return output;
  }
  output.turn = clamp(kApproachKp * error, -kMaxApproachTurn, kMaxApproachTurn);
  output.forward = clamp(100.0 * (stopSize - observation.size) / stopSize,
                         kMinForward, kMaxForward);
  return output;
}
