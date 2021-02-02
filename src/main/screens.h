#pragma once

namespace screens {
  enum Screen {
    WATCHFACE = 0,
    NAP = 1,
  };

  extern  Screen current;

  void dispatch(bool wakeFromSleep);
}
