#pragma once

#include <DS3232RTC.h>

namespace rtc {
  extern DS3232RTC RTC;
  extern tmElements_t currentTime;

  void init();

  void updateTime();

  void resetAlarm();
}
