#include "watchface.h"
#include <thread>
using namespace watchface;

#include "e_ink.h"
using namespace e_ink;

#include "rtc.h"
using namespace rtc;

#include "accelerometer.h"
#include "battery.h"
#include "ble/update_time.h"
#include "notifications.h"

#include "res/icons.h"
#include "res/nums.h"
#include "res/fonts/watchface_font8pt7b.h"

#include <string>
#include <sstream>
#include <set>
#include <vector>

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

void drawNotifications(const std::vector<std::shared_ptr<const Notification>>& notifications) {
  display.fillRect(0, 155, 200, 20, BG_COLOR);

  if(notifications.empty()) {
    return;
  }

  std::set<const unsigned char*> icons;
  for(const auto& notification : notifications) {
    icons.emplace(notification->icon20x20());
  }

  const int margin = 2;
  int x = 100 - (20/2 + margin) * icons.size();
  for(const auto icon : icons) {
    display.drawBitmap(x, 155, icon, 20, 20, FG_COLOR);
    x += 20 + margin;
  }
}

using namespace screens;

#include "constants.h"
#include "event_queue.h"

Screen watchface::update(bool wakeFromSleep /*= true*/) {
  while(auto event = event_queue::queue.receive()) {
    switch(*event) {
      case event_queue::Event::MENU_BUTTON:
        return MENU;
      case event_queue::Event::DOWN_BUTTON:
        return NOTIFICATIONS;
      default:
        break;
    }
  }
  auto now = rtc::currentTime();

  if(wakeFromSleep && now.Hour == 1 && now.Minute == 0) {
    return NAP;
  }

  std::optional<std::thread> bleThread;
  auto notifications = Notification::all();
  if(wakeFromSleep) {
    // REBOOT will also update time, not just notifications
    bleThread = std::thread([](){
        ble::updateTime(rtc::initialized ? ble::FAST_UPDATE : ble::REBOOT);
        });
  }

  display.fillScreen(BG_COLOR);

  display.setTextColor(FG_COLOR);
  display.setFont(&watchface_font8pt7b);

  drawBattery();
  drawDate(now);
  drawTime(now);
  drawStepCount();
  drawNotifications(notifications);

  display.display(wakeFromSleep);
  display.hibernate();

  if(bleThread) {
    bleThread->join();
    bleThread = {};
    auto newNotifications = Notification::all();
    if(newNotifications.size() != notifications.size()
        || !std::equal(
          notifications.begin(),
          notifications.end(),
          newNotifications.begin(),
          [](const std::shared_ptr<const Notification>& a, const std::shared_ptr<const Notification>& b) -> bool { return a->id == b->id; })) {
      // notifications changed, redraw
      drawNotifications(newNotifications);
      display.display(true);
      display.hibernate();
    }
  }

  return WATCHFACE;
}


