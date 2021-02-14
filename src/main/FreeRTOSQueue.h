#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <optional>

template <class T>
class FreeRTOSQueue {
  QueueHandle_t queue;

  public:
  FreeRTOSQueue(BaseType_t length = 10) {
    queue = xQueueCreate(length, sizeof(T));
  }

  ~FreeRTOSQueue() {
    vQueueDelete(queue);
  }

  std::optional<T> receive(TickType_t ticksToWait = 0) {
    T t;
    if(xQueueReceive(queue, &t, ticksToWait)) {
      return t;
    }
    else {
      return {};
    }
  }

  std::optional<T> peek(TickType_t ticksToWait = 0) {
    T t;
    if(xQueuePeek(queue, &t, ticksToWait)) {
      return t;
    }
    else {
      return {};
    }
  }

  bool pushBack(const T& t, TickType_t ticksToWait = 0) {
    return xQueueSendToBack(queue, &t, ticksToWait);
  }
};
