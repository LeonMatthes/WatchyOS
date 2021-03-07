#include "notification_list.h"
#include "screens.h"

#include <cstddef>
#include <sstream>

#include <esp_attr.h>

#include <e_ink.h>
using namespace e_ink;

#include <event_queue.h>
#include <notifications.h>

#include <res/fonts/Inconsolata_Bold10pt7b.h>
#include <res/fonts/Inconsolata_Bold7pt7b.h>
#include <res/fonts/watchface_font8pt7b.h>

void displayNoNotifications() {
  display.fillScreen(BG_COLOR);

  auto headerHeight = screens::drawHeader();

  display.setFont(&Inconsolata_Bold10pt7b);
  display.setCursor(20, 100 + headerHeight / 2);
  display.print("No notifications");

  display.display();
  display.hibernate();
}

void displayEndOfNotifications() {
  display.fillScreen(BG_COLOR);
  auto headerHeight = screens::drawHeader();

  display.setFont(&Inconsolata_Bold10pt7b);
  const char* line1 = "no more";
  const char* line2 = "notifications";

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(line1, 10, 50, &x1, &y1, &w, &h);
  display.setCursor(100 - w/2, (200 - headerHeight) / 2);
  display.println(line1);

  display.getTextBounds(line2, 10, 50, &x1, &y1, &w, &h);
  display.setCursor(100 - w/2, display.getCursorY());
  display.println(line2);

  display.display();
  display.hibernate();
}

void displayNotification(const Notification& notification) {
  display.fillScreen(BG_COLOR);

  auto headerHeight = screens::drawHeader();

  const auto margin = 4;
  display.drawBitmap(margin, headerHeight + margin, notification.icon20x20(), 20, 20, FG_COLOR);

  display.setCursor(20 + 2 * margin, headerHeight + 20 + margin - 4);
  display.setTextWrap(false);
  display.setFont(&Inconsolata_Bold10pt7b);

  display.println(notification.title.c_str());

  display.setTextWrap(true);
  display.setFont(&Inconsolata_Bold7pt7b);
  display.println(notification.text.c_str());


  std::ostringstream timeStream;
  auto& when = notification.when;
  timeStream << when.Hour / 10 << when.Hour % 10 << ":" << when.Minute / 10 << when.Minute % 10;
  std::string time = timeStream.str();

  display.setFont(&watchface_font8pt7b);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(time.c_str(), 10, 50, &x1, &y1, &w, &h);
  display.setCursor(100 - w / 2, 200 - margin);
  display.print(time.c_str());

  display.display();
  display.hibernate();
}

screens::Screen notificationsScreen(bool wakeFromSleep) {

  size_t index = 0;
  while(true) {
    auto notifications = Notification::all();

    if(notifications.empty()) {
      displayNoNotifications();
    }
    else if(index >= notifications.size()) {
      index = notifications.size();
      displayEndOfNotifications();
    }
    else {
      displayNotification(*notifications[index]);
    }

    auto event = event_queue::queue.receive(30 * 1000 / portTICK_PERIOD_MS);
    if(event) {
      switch(*event) {
        case event_queue::Event::BACK_BUTTON:
          return screens::WATCHFACE;
        case event_queue::Event::MENU_BUTTON:
          // TODO
          break;
        case event_queue::Event::DOWN_BUTTON:
          index++;
          break;
        case event_queue::Event::UP_BUTTON:
          if(index == 0) {
            return screens::WATCHFACE;
          }
          index--;
          break;
      }
    }
    else {
      return screens::WATCHFACE;
    }
  }
}
