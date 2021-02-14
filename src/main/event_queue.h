#pragma once

#include "FreeRTOSQueue.h"

namespace event_queue {
  enum class Event {
    MENU_BUTTON,
    BACK_BUTTON,
    UP_BUTTON,
    DOWN_BUTTON,
  };

  extern FreeRTOSQueue<Event> queue;

  void pushWakeupCause();
}
