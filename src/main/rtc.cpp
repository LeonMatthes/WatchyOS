#include "rtc.h"
using namespace rtc;

DS3232RTC rtc::RTC(false);

RTC_DATA_ATTR bool rtc::initialized = false;

time_t compileTime()
{
  const time_t FUDGE(10);    //fudge factor to allow for upload time, etc. (seconds, YMMV)
  const char *compDate = __DATE__, *compTime = __TIME__, *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char compMon[3], *m;

  strncpy(compMon, compDate, 3);
  compMon[3] = '\0';
  m = strstr(months, compMon);

  tmElements_t tm;
  tm.Month = ((m - months) / 3 + 1);
  tm.Day = atoi(compDate + 4);
  tm.Year = atoi(compDate + 7) - 1970;
  tm.Hour = atoi(compTime);
  tm.Minute = atoi(compTime + 3);
  tm.Second = atoi(compTime + 6);

  time_t t = makeTime(tm);
  return t + FUDGE;        //add fudge factor to allow for compile time
}

void rtc::init() {
  RTC.squareWave(SQWAVE_NONE);
  RTC.set(compileTime());
  RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0);
  RTC.alarmInterrupt(ALARM_2, true);
  tmElements_t currentTime;
  RTC.read(currentTime);
  ESP_LOGI("RTC", "Time set to: %i:%i\n", currentTime.Hour, currentTime.Minute);
}

void rtc::setTime(tmElements_t newTime) {
  RTC.set(makeTime(newTime));
}

tmElements_t rtc::currentTime() {
  tmElements_t now{};
  RTC.read(now);
  return now;
}

void rtc::resetAlarm() {
  RTC.alarm(ALARM_2);
}
