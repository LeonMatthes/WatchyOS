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

#include "screens.h"



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
    case ESP_SLEEP_WAKEUP_EXT0:
      rtc::updateTime();
      screens::dispatch(true);
      break;
    default:
      bootup();
      screens::dispatch(false);
      break;
  }

  rtc::resetAlarm();
  accelerometer::clearInterrupts();
  fflush(stdout);
  esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
  esp_deep_sleep_start();
}

