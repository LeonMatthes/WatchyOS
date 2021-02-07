#include "boot.h"

#include <accelerometer.h>
#include <e_ink.h>
using namespace e_ink;
#include <rtc.h>
#include <ble/update_time.h>

#include <res/fonts/Inconsolata_Bold7pt7b.h>

void showBootScreen() {
  display.fillScreen(GxEPD_BLACK);

  display.setTextColor(GxEPD_WHITE);
  display.setFont(&Inconsolata_Bold7pt7b);

  display.setCursor(0, 20);
  display.println("Booting WatchyOS...");

  display.display();
}

void initBMA() {
  bool success =
    accelerometer::init() &&
    accelerometer::setFeature(BMA423_STEP_CNTR, true) &&
    accelerometer::setFeature(BMA423_WRIST_WEAR, true) &&
    accelerometer::setFeatureInterrupt(BMA4_INTR1_MAP, BMA423_WRIST_WEAR_INT, true);

  if(!success) {
    display.fillScreen(GxEPD_BLACK);

    display.setTextColor(GxEPD_WHITE);
    display.setFont(&Inconsolata_Bold7pt7b);

    display.setCursor(0, 20);
    display.println("Accelerometer initialization failed!");

    display.display(false);
    display.hibernate();

    fflush(stdout);
    // deep sleep without wakeup source to disallow further bootup
    esp_deep_sleep_start();
  }
}

screens::Screen bootScreen(bool _wakeFromSleep) {
  showBootScreen();

  display.print("Init RTC...");
  display.display(true);
  rtc::init();
  display.println("Done");
  display.display(true);

  display.print("Connecting to phone...");
  display.display(true);
  bool timeUpdated = ble::updateTime(30'000'000 /*ns*/);
  display.println(timeUpdated ? "Done" : "Failed");
  display.display(true);

  display.print("Init BMA423...");
  display.display(true);
  initBMA();
  display.println("Done");
  display.display(true);


  return screens::WATCHFACE;
}
