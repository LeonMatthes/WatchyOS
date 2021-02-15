#include "timer.h"
#include "screens.h"

#include <event_queue.h>
using namespace event_queue;
#include <res/nums.h>

#include <e_ink.h>
using namespace e_ink;

int drawTime(int minutes, int seconds) {
    display.fillScreen(BG_COLOR);

    auto headerHeight = screens::drawHeader();

    auto y = headerHeight + (200 - headerHeight) / 2 - 30;
    display.drawBitmap(27, y, nums[minutes / 10], 30, 60, FG_COLOR);
    display.drawBitmap(27 + 35, y, nums[minutes % 10], 30, 60, FG_COLOR);

    display.fillRect(97, y + 15, 6, 10, FG_COLOR);
    display.fillRect(97, y + 35, 6, 10, FG_COLOR);

    display.drawBitmap(108, y, nums[seconds / 10], 30, 60, FG_COLOR);
    display.drawBitmap(108 + 35, y, nums[seconds % 10], 30, 60, FG_COLOR);
    return y;
}

std::optional<int64_t> selectTime() {
  auto minutes = 5;
  auto seconds = 0;
  bool selectMinutes = true;

  bool initial = true;
  do {
    auto y = drawTime(minutes, seconds);

    display.fillRect(selectMinutes ? 27 : 108, y + 65, 65, 5, FG_COLOR);

    display.display(!initial);
    display.hibernate();


    do {
      auto event = queue.receive(portMAX_DELAY);
      if(event) {
        if(selectMinutes) {
          switch (*event) {
            case Event::MENU_BUTTON:
              selectMinutes = false;
              break;
            case Event::BACK_BUTTON:
              return {};
              break;
            case Event::UP_BUTTON:
              minutes = (minutes + 1) % 100;
              break;
            case Event::DOWN_BUTTON:
              minutes = (minutes <= 0) ? 99 : minutes - 1;
              break;
          }
        }
        else {
          switch(*event) {
            case Event::MENU_BUTTON:
              return minutes * 60 + seconds;
            case Event::BACK_BUTTON:
              selectMinutes = true;
              break;
            case Event::UP_BUTTON:
              seconds = (seconds + 1) % 60;
              break;
            case Event::DOWN_BUTTON:
              seconds = (seconds <= 0) ? 59 : seconds - 1;
              break;
          }
        }
      }
    }
    while(queue.peek());

    initial = false;
  }
  while(true);
}

#include <vibration_motor.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <res/fonts/Inconsolata_Bold10pt7b.h>

bool pauseTimer(uint64_t remainingTime) {
  auto remainingSeconds = remainingTime / (1000 * 1000);
  auto y = drawTime(remainingSeconds / 60, remainingSeconds % 60);
  display.setFont(&Inconsolata_Bold10pt7b);

  auto paused = "Paused";

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(paused, 0, y + 70, &x1, &y1, &w, &h);
  display.setCursor(100 - w / 2, y + 65 + 20);
  display.print(paused);
  display.display(true);

  while(true) {
    auto event = queue.receive(portMAX_DELAY);
    if(event) {
      switch(*event) {
        case Event::BACK_BUTTON:
          return false;
        case Event::MENU_BUTTON:
          return true;
        default:
          break;
      }
    }
  }
}

screens::Screen timerScreen(bool wakeFromSleep) {

  auto selected = selectTime();
  if(!selected) {
    return screens::MENU;
  }

  vibration_motor::delay(300);
  vibration_motor::buzz(50);
  vibration_motor::delay(300);
  vibration_motor::buzz(50);

  // esp_get_time function is in ms
  auto timer = *selected * 1000 * 1000;
  auto beginTime = esp_timer_get_time();

  const auto drawDurationNs = 300 * 1000; /*around the time it takes for partial refresh*/

  int64_t remainingSeconds;
  do {
    auto remainingTime = timer - (esp_timer_get_time() - beginTime);

    while(queue.peek()) {
      auto event = queue.receive();
      if(event) {
        switch(*event) {
          case Event::BACK_BUTTON:
            return screens::MENU;
          case Event::MENU_BUTTON:
            if(pauseTimer(remainingTime)) {
              beginTime = esp_timer_get_time();
              timer = remainingTime;
              remainingTime = timer - (esp_timer_get_time() - beginTime);
            }
            else {
              return screens::MENU;
            }
          default:
            break;
        }
      }
    }

    remainingSeconds = remainingTime / (1000 * 1000);
    auto nextSecondTime = remainingSeconds * 1000 * 1000;
    auto sleepTime = remainingTime - nextSecondTime - drawDurationNs;
    if(sleepTime > 0) {
      vTaskDelay((sleepTime / 1000) / portTICK_PERIOD_MS);
    }

    remainingTime = timer - (esp_timer_get_time() - beginTime);
    remainingSeconds = remainingTime / (1000 * 1000);
    if(remainingSeconds < 0) {
      remainingSeconds = 0;
    }

    // we need to queue the vibration before we actually draw the new time.
    // Otherwise it's noticeable, that it vibrates AFTER the time is displayed
    // This might be due to a delay in how the screen is drawn,
    // or maybe how long it takes for the vibration to start
    // Luckily the vibration motor is asynchronous
    if(remainingSeconds <= 3) {
      vibration_motor::delay(drawDurationNs / 1000);
      if(remainingSeconds > 0) {
        vibration_motor::buzz(50);
      }
      else {
        // vibration for hitting zero
        for(int i = 0; i < 3; i++) {
          vibration_motor::buzz(100);
          vibration_motor::delay(300);
        }
      }
    }
    drawTime(remainingSeconds / 60, remainingSeconds % 60);
    // use full refresh for effect when time is up
    display.display(remainingSeconds > 0);
  }
  while(remainingSeconds > 0);

  do {
    auto event = queue.receive(portMAX_DELAY);
    if(event){
      switch(*event) {
        case Event::BACK_BUTTON:
          return screens::MENU;
        case Event::MENU_BUTTON:
          return timerScreen(false);
        default:
          break;
      }

    }
  }
  while(true);
}
