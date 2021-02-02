#include "watchface.h"
using namespace watchface;

#include "e_ink.h"
using namespace e_ink;

#include "rtc.h"
using namespace rtc;

#include "accelerometer.h"
#include "battery.h"

#include "res/icons.h"
#include "res/nums.h"
#include "res/fonts/watchface_font8pt7b.h"

#include <string>
#include <sstream>

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

void drawStepCount() {
  display.drawBitmap(54, 180, icon_steps, 13, 15, GxEPD_WHITE);

  uint32_t stepCount = accelerometer::stepCount();
  std::string steps = std::to_string(stepCount);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(steps.c_str(), 70, 193, &x1, &y1, &w, &h);
  display.setCursor(100 - (w/2), 193);
  display.println(steps.c_str());

  if(stepCount >= 10000) {
    display.drawBitmap(131, 180, icon_star, 15, 15, GxEPD_WHITE);
  }
}

using namespace screens;

#include "constants.h"

Screen watchface::update(bool partial_refresh /*= true*/) {
  if((esp_sleep_get_ext1_wakeup_status() & MENU_BTN_MASK)
      || (rtc::currentTime.Hour == 1 && rtc::currentTime.Minute == 0)) {
    return NAP;
  }

  display.fillScreen(GxEPD_BLACK);

  display.setTextColor(GxEPD_WHITE);
  display.setFont(&watchface_font8pt7b);

  drawBattery();
  drawDate();
  drawTime();
  drawStepCount();

  display.display(partial_refresh);
  display.hibernate();
  return WATCHFACE;
}


