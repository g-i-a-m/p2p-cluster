#ifndef SHARK_LIB_CLOCKUTILS_H_
#define SHARK_LIB_CLOCKUTILS_H_

#include <chrono>  // NOLINT

#include "Clock.h"

namespace shark {

class ClockUtils {
 public:
  static inline int64_t durationToMs(shark::duration duration) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  }

  static inline uint64_t timePointToMs(shark::time_point time_point) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count();
  }
};

}  // namespace shark
#endif  // SHARK_LIB_CLOCKUTILS_H_
