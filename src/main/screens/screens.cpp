#include "screens.h"

#include "esp_attr.h"

#include <functional>

#include "watchface.h"
#include "boot.h"
#include "timer.h"

#include "screens_menu.h"
#include "notification_list.h"
#include "testing.h"
#include <event_queue.h>

using namespace screens;

Screen takeNap(bool wake_from_sleep);

RTC_DATA_ATTR Screen screens::current = BOOT;

const std::function<Screen(bool)> screenFunctions[] = {
  watchface::update,
  takeNap,
  screensMenu,
  testScreen,
  bootScreen,
  timerScreen,
  notificationsScreen
};


void screens::dispatch(bool wakeFromSleep) {
  Screen next = current;
  do {
    current = next;
    next = screenFunctions[current](wakeFromSleep);
    wakeFromSleep = false;
  }
  while(next != current || event_queue::queue.peek());
}

#include <constants.h>

#include <res/fonts/Inconsolata_Bold7pt7b.h>
#include <res/icons.h>
#include <e_ink.h>
using namespace e_ink;

#include <rtc.h>
#include <accelerometer.h>
#include <event_queue.h>

Screen takeNap(bool wake_from_sleep = false) {
  if(wake_from_sleep) {
    // pop the event that woke us from the queue
    event_queue::queue.receive();
    return WATCHFACE;
  }

  display.fillScreen(GxEPD_BLACK);

  display.setTextColor(GxEPD_WHITE);

  display.drawBitmap(50, 20, icon_moon, 100, 100, GxEPD_WHITE);


  int16_t x1, y1;
  uint16_t w, h;
  display.setFont(&Inconsolata_Bold7pt7b);
  const char* line1 = "Watchy is sleeping";
  display.getTextBounds(line1, 0, 150, &x1, &y1, &w, &h);
  display.setCursor(100 - w/2, 150);
  display.print(line1);

  // necessary, as otherwise the compiler seems to optimize
  // some code away, which breaks width calculation for line1
  w = 0;
  h = 0;
  const char* line2 = "Press any button to wake up";
  display.getTextBounds(line2, 0, 150, &x1, &y1, &w, &h);
  display.setCursor(100 - w/2, 170);
  display.print(line2);

  display.display();
  display.hibernate();

  //enable deep sleep wake on button press, and nothing else
  rtc::resetAlarm();
  accelerometer::clearInterrupts();
  fflush(stdout);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_deep_sleep_start();
}

#include "res/fonts/watchface_font8pt7b.h"
#include <sstream>

int16_t screens::drawHeader() {
  tmElements_t now = rtc::currentTime();
  display.setTextColor(FG_COLOR);
  display.setFont(&watchface_font8pt7b);
  display.setCursor(5, 15);

  std::ostringstream timeStream;
  timeStream << now.Hour / 10 << now.Hour % 10 << ":" << now.Minute / 10 << now.Minute % 10;
  display.print(timeStream.str().c_str());


  std::ostringstream dateStream;
  dateStream << now.Day / 10 << now.Day % 10 << "." << now.Month / 10 << now.Month % 10 << ".";
  std::string date = dateStream.str();

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(date.c_str(), 10, 50, &x1, &y1, &w, &h);
  display.setCursor(195 - w, 15);
  display.print(date.c_str());

  display.drawFastHLine(0, 20, 200, FG_COLOR); 

  return 20;
}
