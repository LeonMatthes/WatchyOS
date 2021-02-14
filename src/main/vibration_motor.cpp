#include "vibration_motor.h"
#include <thread>
using namespace vibration_motor;

#include "FreeRTOSQueue.h"

static FreeRTOSQueue<int> commands;

bool vibration_motor::delay(int ms) {
  return commands.pushBack(-ms);
}

bool vibration_motor::buzz(int ms) {
  return commands.pushBack(ms);
}

#include "constants.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/task.h"

void vibrationMotorTask() {
  // this is an endless loop
  while(auto command = commands.receive(portMAX_DELAY)) {
    // when exiting light sleep, the pin must be reconfigured
    gpio_pad_select_gpio(VIB_MOTOR_GPIO);
    gpio_set_direction(VIB_MOTOR_GPIO, GPIO_MODE_OUTPUT);

    if(*command < 0) {
      gpio_set_level(VIB_MOTOR_GPIO, 0);
      vTaskDelay(-(*command) / portTICK_PERIOD_MS);
    }
    else {
      gpio_set_level(VIB_MOTOR_GPIO, 1);
      vTaskDelay(*command / portTICK_PERIOD_MS);
      gpio_set_level(VIB_MOTOR_GPIO, 0);
    }
  }
  ESP_LOGE("Vibration Motor", "Vibration motor reached end of task");
}

std::thread vibration_motor::startTask() {
  return std::move(std::thread(vibrationMotorTask));
}
