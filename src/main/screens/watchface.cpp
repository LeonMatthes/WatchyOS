#include "watchface.h"
using namespace watchface;

#include "e_ink.h"
using namespace e_ink;

#include "rtc.h"
using namespace rtc;

#include "accelerometer.h"
#include "battery.h"
#include "ble/update_time.h"

#include "res/icons.h"
#include "res/nums.h"
#include "res/fonts/watchface_font8pt7b.h"

#include <string>
#include <sstream>

void drawBattery() {
  display.drawBitmap(5, 5, icon_battery, 45, 15, FG_COLOR);
  display.fillRect(8, 8, 37 * batteryPercentage(), 9, FG_COLOR);

  display.setCursor(55, 19);
  display.print(batteryVoltage());
  display.println("V");
}

void drawDate(const tmElements_t& now) {

  std::ostringstream dateStream;
  dateStream << now.Day / 10 << now.Day % 10 << "." << now.Month / 10 << now.Month % 10 << ".";
  std::string date = dateStream.str();

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(date.c_str(), 10, 50, &x1, &y1, &w, &h);
  display.setCursor(195 - w, 19);
  display.print(date.c_str());
}

void drawTime(const tmElements_t& now) {
  display.drawBitmap(67, 25, nums[now.Hour / 10], 30, 60, FG_COLOR);
  display.drawBitmap(103, 25, nums[now.Hour % 10], 30, 60, FG_COLOR);
  display.drawBitmap(67, 90, nums[now.Minute / 10], 30, 60, FG_COLOR);
  display.drawBitmap(103, 90, nums[now.Minute % 10], 30, 60, FG_COLOR);
}

void drawStepCount() {
  display.drawBitmap(54, 180, icon_steps, 13, 15, FG_COLOR);

  uint32_t stepCount = accelerometer::stepCount();
  std::string steps = std::to_string(stepCount);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(steps.c_str(), 70, 193, &x1, &y1, &w, &h);
  display.setCursor(100 - (w/2), 193);
  display.println(steps.c_str());

  if(stepCount >= 10000) {
    display.drawBitmap(131, 180, icon_star, 15, 15, FG_COLOR);
  }
}

void drawNotifications() {
  using namespace ble;
  if(notifications & Notifications::WHATSAPP) {
    display.drawBitmap(90, 155, icon_whatsapp, 20, 20, FG_COLOR);
  }
}

using namespace screens;

#include "constants.h"

Screen watchface::update(bool wakeFromSleep /*= true*/) {
  if(wakeFromSleep && (esp_sleep_get_ext1_wakeup_status() & MENU_BTN_MASK)) {
    return MENU;
  }

  if(wakeFromSleep) {
    // REBOOT will also update time, not just notifications
    ble::updateTime(rtc::initialized ? ble::FAST_UPDATE : ble::REBOOT);
  }

  tmElements_t now = rtc::currentTime();

  if(wakeFromSleep && now.Hour == 1 && now.Minute == 0) {
    return NAP;
  }

  display.fillScreen(BG_COLOR);

  display.setTextColor(FG_COLOR);
  display.setFont(&watchface_font8pt7b);

  drawBattery();
  drawDate(now);
  drawTime(now);
  drawStepCount();
  drawNotifications();

  display.display(wakeFromSleep);
  display.hibernate();
  return WATCHFACE;
}


