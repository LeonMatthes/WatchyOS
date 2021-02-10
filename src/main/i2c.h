#pragma once

#include <mutex>

namespace i2c {
  // acquire this mutex before doing any kind of i2c operation
  extern std::mutex mutex;
}
