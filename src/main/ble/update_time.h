#pragma once

#include <cstdint>

#include <TimeLib.h>

namespace ble {

  // data must contain at least 7 bytes
  TimeElements timeFromBytes(const char* data);

  enum Notifications {
    WHATSAPP = 0x1
  };

  enum State {
    REBOOT = 1, // should update time and notifications
    FAST_UPDATE = 2,
    CONNECTION = 3,
    DISCONNECT = 0xFF // only ever sent by the Phone to disconnect
  };

  bool updateTime(State connectionState = FAST_UPDATE, int64_t timeout = 3'000'000 /*ns*/);
}
