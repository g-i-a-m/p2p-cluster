
#include "TokenBucket.h"

#include <cassert>
#include <iostream>

namespace shark {

TokenBucket::TokenBucket(std::shared_ptr<shark::Clock> the_clock)
  : time_{0},
    time_per_token_{0},
    time_per_burst_{0},
    clock_{the_clock} {}

TokenBucket::TokenBucket(const uint64_t rate, const uint64_t burst_size,
            std::shared_ptr<shark::Clock> the_clock)
  : time_ {0},
    time_per_token_{1000000 / std::max(rate, uint64_t(1))},
    time_per_burst_{burst_size * time_per_token_},
    clock_{the_clock} {}

TokenBucket::TokenBucket(const TokenBucket &other) {
  time_per_token_ = other.time_per_token_.load();
  time_per_burst_ = other.time_per_burst_.load();
  clock_ = other.clock_;
}

TokenBucket& TokenBucket::operator=(const TokenBucket &other) {
  time_per_token_ = other.time_per_token_.load();
  time_per_burst_ = other.time_per_burst_.load();
  clock_ = other.clock_;
  return *this;
}

void TokenBucket::reset(const uint64_t rate, const uint64_t burst_size) {
  time_per_token_ = 1000000 / std::max(rate, uint64_t(1));
  time_per_burst_ = burst_size * time_per_token_;
  time_ = 0;
}

bool TokenBucket::consume(const uint64_t tokens) {
  const uint64_t now = std::chrono::duration_cast<std::chrono::microseconds>(
            clock_->now().time_since_epoch())
            .count();
  const uint64_t time_needed = tokens * time_per_token_.load(std::memory_order_relaxed);
  const uint64_t min_time = now - time_per_burst_.load(std::memory_order_relaxed);
  uint64_t old_time = time_.load(std::memory_order_relaxed);
  uint64_t new_time = old_time;

  if (min_time > old_time) {
    new_time = min_time;
  }

  for (;;) {
    new_time += time_needed;
    if (new_time > now) {
      return false;
    }
    if (time_.compare_exchange_weak(old_time, new_time,
                                    std::memory_order_relaxed,
                                    std::memory_order_relaxed)) {
      return true;
    }
    new_time = old_time;
  }

  return false;
}

}  // namespace shark
