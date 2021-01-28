#include <sstream>
#include <stdio.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <DS3232RTC.h>
#include <Wire.h>

#include <cmath>
#include <string>
#include <sstream>

#include "constants.h"
#include "battery.h"

#include "res/icons.h"
#include "res/nums.h"
#include "res/fonts/watchface_font8pt7b.h"



GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(CS, DC, RESET, BUSY_PIN));
DS3232RTC RTC(false);
tmElements_t currentTime;

extern "C" void app_main(void);

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


void initRTC() {
  RTC.squareWave(SQWAVE_NONE);
  RTC.set(compileTime());
  RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0);
  RTC.alarmInterrupt(ALARM_2, true);
  RTC.read(currentTime);
}

void drawBattery() {
  display.drawBitmap(5, 5, icon_battery, 45, 15, GxEPD_WHITE);
  display.fillRect(8, 8, 37 * batteryPercentage(), 9, GxEPD_WHITE);

  display.setCursor(55, 19);
  display.print(batteryVoltage());
  display.println("V");
}

void drawDate() {
  std::ostringstream dateStream;
  dateStream << currentTime.Day / 10 << currentTime.Day % 10 << "." << currentTime.Month / 10 << currentTime.Month % 10 << ".";
  std::string date = dateStream.str();

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(date.c_str(), 10, 50, &x1, &y1, &w, &h);
  display.setCursor(195 - w, 19);
  display.print(date.c_str());
}

void drawTime() {
  display.drawBitmap(67, 25, nums[currentTime.Hour / 10], 30, 60, GxEPD_WHITE);
  display.drawBitmap(103, 25, nums[currentTime.Hour % 10], 30, 60, GxEPD_WHITE);
  display.drawBitmap(67, 90, nums[currentTime.Minute / 10], 30, 60, GxEPD_WHITE);
  display.drawBitmap(103, 90, nums[currentTime.Minute % 10], 30, 60, GxEPD_WHITE);
}

void updateDisplay(bool partial_refresh) {
  display.init(0, false);
  display.setFullWindow();
  display.fillScreen(GxEPD_BLACK);

  display.setTextColor(GxEPD_WHITE);
  display.setFont(&watchface_font8pt7b);

  drawBattery();
  drawDate();
  drawTime();

  display.display(partial_refresh);
  display.hibernate();
}

void app_main(void)
{
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    gpio_pad_select_gpio(VIB_MOTOR_GPIO);
    gpio_set_direction(VIB_MOTOR_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_level(VIB_MOTOR_GPIO, 1);

    vTaskDelay(50 / portTICK_PERIOD_MS);

    gpio_set_level(VIB_MOTOR_GPIO, 0);
  }

  initArduino();
  Wire.begin(SDA_PIN, SCL_PIN);

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      printf("RTC wakeup\n");
      __attribute__ ((fallthrough));
    case ESP_SLEEP_WAKEUP_EXT1:
      RTC.alarm(ALARM_2);
      RTC.read(currentTime);
      updateDisplay(true);
      break;
    default:
      initRTC();
      updateDisplay(false);
      break;
  }



  fflush(stdout);
  esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
  esp_deep_sleep_start();
}

