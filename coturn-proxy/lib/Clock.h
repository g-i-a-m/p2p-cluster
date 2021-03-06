#ifndef SHARK_LIB_CLOCK_H_
#define SHARK_LIB_CLOCK_H_

#include <chrono>  // NOLINT

namespace shark {

using clock = std::chrono::steady_clock;
using time_point = std::chrono::steady_clock::time_point;
using duration = std::chrono::steady_clock::duration;

class Clock {
 public:
  virtual time_point now() = 0;
};

class SteadyClock : public Clock {
 public:
  time_point now() override {
    return clock::now();
  }
};

class SimulatedClock : public Clock {
 public:
  SimulatedClock() : now_{clock::now()} {}

  time_point now() override {
    return now_;
  }

  void advanceTime(duration duration) {
    now_ += duration;
  }
 private:
  time_point now_;
};
}  // namespace shark
#endif  // SHARK_LIB_CLOCK_H_
