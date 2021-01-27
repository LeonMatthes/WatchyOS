#include <stdio.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "constants.h"

void app_main(void)
{
  gpio_config_t io_conf;
  // disable interrupts
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = 1ULL << VIB_MOTOR_PIN;
  //disable pull-down mode
  io_conf.pull_down_en = 0;
  //disable pull-up mode
  io_conf.pull_up_en = 0;
  //configure GPIO with the given settings
  gpio_config(&io_conf);

  gpio_set_level(VIB_MOTOR_PIN, 1);

  vTaskDelay(50 / portTICK_PERIOD_MS);

  gpio_set_level(VIB_MOTOR_PIN, 0);

  /* esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt */
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
  esp_deep_sleep_start();
}
