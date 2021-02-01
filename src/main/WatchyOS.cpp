#include <sstream>
#include <stdio.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Arduino.h>
#include <Wire.h>

#include <cmath>

#include "constants.h"
#include "accelerometer.h"
#include "rtc.h"


#include "watchface.h"
#include "res/fonts/Inconsolata_Bold7pt7b.h"
#include "e_ink.h"
using namespace e_ink;

#include "res/icons.h"

enum class MenuState {
  WATCHFACE,
  NAP
};

RTC_DATA_ATTR  MenuState currentMenu = MenuState::WATCHFACE;

extern "C" void app_main(void);

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

void bootup() {
  rtc::init();
  showBootScreen();

  display.print("Init BMA423...");
  display.display(true);
  initBMA();
  display.print("Done");
  display.display(true);
}

void takeNap() {
  currentMenu = MenuState::NAP;

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
  Wire.setClock(400'000);

  display.init(0, false);

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT1:
      if(currentMenu == MenuState::WATCHFACE && esp_sleep_get_ext1_wakeup_status() & MENU_BTN_MASK) {
        takeNap();
        break;
      }
      __attribute__ ((fallthrough));
    case ESP_SLEEP_WAKEUP_EXT0:
      rtc::updateTime();
      if(rtc::currentTime.Hour == 1 && rtc::currentTime.Minute == 0) {
        takeNap();
        break;
      }
      watchface::update(currentMenu == MenuState::WATCHFACE);
      currentMenu = MenuState::WATCHFACE;
      break;
    default:
      bootup();
      watchface::update(false);
      break;
  }

  rtc::resetAlarm();
  accelerometer::clearInterrupts();
  fflush(stdout);
  esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
  esp_deep_sleep_start();
}

