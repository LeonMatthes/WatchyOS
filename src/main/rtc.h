#pragma once

#include <DS3232RTC.h>

namespace rtc {
  extern DS3232RTC RTC;

  void init();

  void setTime(tmElements_t newTime);

  tmElements_t currentTime();

  void resetAlarm();
}
