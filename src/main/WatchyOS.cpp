#include <sstream>
#include <stdio.h>
#include "esp_sleep.h"
#include "esp_pm.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Arduino.h>
#include <Wire.h>

#include <cmath>
#include <string>

#include "constants.h"
#include "accelerometer.h"
#include "rtc.h"
#include "event_queue.h"
#include "vibration_motor.h"
#include "input.h"
#include "notifications.h"


#include "e_ink.h"
using namespace e_ink;

#include "res/fonts/Inconsolata_Bold10pt7b.h"

#include "screens/screens.h"


void showResetCause(esp_reset_reason_t resetReason) {
  display.init(0, false);

  display.fillScreen(BG_COLOR);
  display.setFont(&Inconsolata_Bold10pt7b);
  display.setCursor(20, 20);

  display.print("Reset cause: ");
  display.println(std::to_string(resetReason).c_str());

  display.display();
  display.hibernate();
}

extern "C" void app_main(void);

void app_main(void)
{
  esp_pm_config_esp32_t pm_config = {
    .max_freq_mhz = 240,
    .min_freq_mhz = 40,
    .light_sleep_enable = false
  };

  ESP_ERROR_CHECK(esp_pm_configure(&pm_config));

  vibration_motor::startTask().detach();
  esp_reset_reason_t resetReason = esp_reset_reason();
  if(resetReason == ESP_RST_DEEPSLEEP && esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
    // buzz to acknowledge input
    vibration_motor::buzz(50);
  }
  printf("Reset cause: %i\n", resetReason);

  initArduino();
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400'000);
  display.init(0, false);
  input::startTask().detach();

  switch (resetReason) {
    case ESP_RST_SW:
    case ESP_RST_POWERON:
      screens::current = screens::BOOT;
      screens::dispatch(false);
      break;
    case ESP_RST_DEEPSLEEP:
      event_queue::pushWakeupCause();
      screens::dispatch(true);
      break;
    default:
      showResetCause(resetReason);
      break;
  }

  rtc::resetAlarm();
  accelerometer::clearInterrupts();
  Notification::storeInRTC();
  fflush(stdout);

  // See: https://github.com/sqfmi/Watchy/pull/117
  // for(int i = 0; i < 48; i++) {
    // pinMode(i, INPUT);
  // }

  esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
  ESP_LOGI("WatchyOS", "Entering Deep Sleep");
  esp_deep_sleep_start();
}

