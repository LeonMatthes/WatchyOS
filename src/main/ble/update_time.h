#pragma once

#include <cstdint>

namespace ble {
  enum Notifications {
    WHATSAPP = 0x1
  };

  extern uint8_t notifications;

  bool updateTime(int64_t timeout = 3'000'000 /*ns*/);
}
