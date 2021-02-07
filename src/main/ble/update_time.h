#pragma once

#include <cstdint>

namespace ble {
  bool updateTime(int64_t timeout = 3'000'000 /*ns*/);
}
