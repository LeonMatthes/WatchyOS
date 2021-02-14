#include "input.h"

#include "event_queue.h"
#include <thread>
using namespace event_queue;
#include "constants.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <vibration_motor.h>

// TODO: Refactor using interupts?
// Interrupts would not be compatible with light sleep though
void processPin(uint64_t& pinMask, gpio_num_t gpio, uint64_t gpio_mask, Event event) {
  int level = gpio_get_level(gpio);
  if(level && !(pinMask & gpio_mask)) {
    queue.pushBack(event, portMAX_DELAY);
    vibration_motor::buzz(50);
  }
  // mask away the pin level
  if(level) {
    pinMask |= gpio_mask;
  }
  else {
    pinMask &= ~gpio_mask;
  }
}

void inputTask() {
  gpio_config_t pinConfig = {
    .pin_bit_mask = BTN_PIN_MASK,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&pinConfig);

  uint64_t pinMask = 0;
  if(gpio_get_level(MENU_BTN_GPIO)) {
    pinMask |= MENU_BTN_MASK;
  }
  if(gpio_get_level(BACK_BTN_GPIO)) {
    pinMask |= BACK_BTN_MASK;
  }
  if(gpio_get_level(DOWN_BTN_GPIO)) {
    pinMask |= DOWN_BTN_GPIO;
  }
  if(gpio_get_level(UP_BTN_GPIO)) {
    pinMask |= UP_BTN_GPIO;
  }
  while(true) {
    processPin(pinMask, MENU_BTN_GPIO, MENU_BTN_MASK, Event::MENU_BUTTON);
    processPin(pinMask, BACK_BTN_GPIO, BACK_BTN_MASK, Event::BACK_BUTTON);
    processPin(pinMask, DOWN_BTN_GPIO, DOWN_BTN_MASK, Event::DOWN_BUTTON);
    processPin(pinMask, UP_BTN_GPIO, UP_BTN_MASK, Event::UP_BUTTON);

    // wait for 10ms, then read again
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

std::thread input::startTask() {
  return std::move(std::thread(inputTask));
}
