#include <stdio.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Arduino.h"

#include "constants.h"


void app_main(void)
{
  gpio_pad_select_gpio(VIB_MOTOR_GPIO);
  gpio_set_direction(VIB_MOTOR_GPIO, GPIO_MODE_OUTPUT);

  gpio_set_level(VIB_MOTOR_GPIO, 1);

  vTaskDelay(50 / portTICK_PERIOD_MS);

  gpio_set_level(VIB_MOTOR_GPIO, 0);

  initArduino();

  /* esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt */
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
  esp_deep_sleep_start();
}

