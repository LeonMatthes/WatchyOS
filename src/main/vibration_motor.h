#pragma once

#include <thread>

namespace vibration_motor {
  // using the vibration motor is non-blocking
  bool delay(int ms);
  bool buzz(int ms);

  std::thread startTask();
}
