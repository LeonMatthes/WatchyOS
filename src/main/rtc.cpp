#include "rtc.h"
using namespace rtc;

DS3232RTC rtc::RTC(false);
tmElements_t rtc::currentTime;

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
  RTC.read(currentTime);
}

void rtc::updateTime() {
  RTC.read(currentTime);
}

void rtc::resetAlarm() {
  RTC.alarm(ALARM_2);
}
