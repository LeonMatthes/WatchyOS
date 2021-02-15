#pragma once

#include <cstdint>

namespace screens {
  enum Screen {
    WATCHFACE = 0,
    NAP = 1,
    MENU = 2,
    TEST_SCREEN = 3,
    BOOT = 4,
    TIMER = 5,
  };

  extern Screen current;

  void dispatch(bool wakeFromSleep);

  int16_t drawHeader();
}
