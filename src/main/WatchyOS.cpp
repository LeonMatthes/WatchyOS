#include <sstream>
#include <stdio.h>
#include "esp_sleep.h"
#include "esp_pm.h"
#include "esp_system.h"
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

  initArduino();
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400'000);
  display.init(0, false);

  esp_reset_reason_t resetReason = esp_reset_reason();
  printf("Reset cause: %i\n", resetReason);
  switch (resetReason) {
    case ESP_RST_SW:
    case ESP_RST_POWERON:
      screens::current = screens::BOOT;
      screens::dispatch(false);
      break;
    case ESP_RST_DEEPSLEEP:
      {
        esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

        if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
          gpio_pad_select_gpio(VIB_MOTOR_GPIO);
          gpio_set_direction(VIB_MOTOR_GPIO, GPIO_MODE_OUTPUT);

          gpio_set_level(VIB_MOTOR_GPIO, 1);

          vTaskDelay(50 / portTICK_PERIOD_MS);

          gpio_set_level(VIB_MOTOR_GPIO, 0);
        }

        screens::dispatch(true);
      }
      break;
    default:
      showResetCause(resetReason);
      break;
  }

  rtc::resetAlarm();
  accelerometer::clearInterrupts();
  fflush(stdout);
  esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
  esp_deep_sleep_start();
}

