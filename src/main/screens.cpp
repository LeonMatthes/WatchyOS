#include "screens.h"

#include "esp_attr.h"

#include <functional>

#include "watchface.h"

using namespace screens;

Screen takeNap(bool wake_from_sleep);

RTC_DATA_ATTR Screen screens::current = WATCHFACE;

const std::function<Screen(bool)> screenFunctions[] = {
  watchface::update,
  takeNap
};


void screens::dispatch(bool wakeFromSleep) {
  Screen next = current;
  do {
    current = next;
    next = screenFunctions[current](wakeFromSleep);
    wakeFromSleep = false;
  }
  while(next != current);
}

#include "constants.h"

#include "res/fonts/Inconsolata_Bold7pt7b.h"
#include "res/icons.h"
#include "e_ink.h"
using namespace e_ink;

#include "rtc.h"
#include "accelerometer.h"

Screen takeNap(bool wake_from_sleep = false) {
  if(wake_from_sleep) {
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
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_deep_sleep_start();
}
