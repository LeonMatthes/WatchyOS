#include "event_queue.h"

using namespace event_queue;

FreeRTOSQueue<Event> event_queue::queue;

#include "esp_sleep.h"
#include "constants.h"

void event_queue::pushWakeupCause() {
  uint64_t ext1_cause = esp_sleep_get_ext1_wakeup_status();

  if(ext1_cause & MENU_BTN_MASK) {
    queue.pushBack(Event::MENU_BUTTON);
  }
  if(ext1_cause & BACK_BTN_MASK) {
    queue.pushBack(Event::BACK_BUTTON);
  }
  if(ext1_cause & UP_BTN_MASK) {
    queue.pushBack(Event::UP_BUTTON);
  }
  if(ext1_cause & DOWN_BTN_MASK) {
    queue.pushBack(Event::DOWN_BUTTON);
  }
}
