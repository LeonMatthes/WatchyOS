#include "screens_menu.h"
#include "rtc.h"

using namespace screens;

#include <array>
#include <cstdint>

#include "constants.h"
#include "res/fonts/Inconsolata_Bold10pt7b.h"
#include "e_ink.h"
using namespace e_ink;

struct MenuEntry {
  Screen screen;
  const char* name;
};

const std::array<MenuEntry, 3> entries = {{
  {
    .screen = NAP,
    .name = "Go to sleep"
  },
  {
    .screen = TEST_SCREEN,
    .name = "Test screen"
  },
  {
    .screen = BOOT,
    .name = "Reboot"
  }
}};

screens::Screen screensMenu(bool wakeFromSleep) {
  int selectedIndex = 0;
  const int margin = 2;

  // only partial refresh when waking up again
  bool initial = true;
  while(true) {

    if(!initial) {

      uint64_t btn_presses = esp_sleep_get_ext1_wakeup_status();

      if(btn_presses) {
        gpio_set_level(VIB_MOTOR_GPIO, 1);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_set_level(VIB_MOTOR_GPIO, 0);
      }

      if(btn_presses & MENU_BTN_MASK) {
        return entries[selectedIndex].screen;
      }

      if(btn_presses & BACK_BTN_MASK) {
        return WATCHFACE;
      }

      if(btn_presses & DOWN_BTN_MASK) {
        selectedIndex++;
        selectedIndex %= entries.size();
      }

      if(btn_presses & UP_BTN_MASK) {
        selectedIndex--;
        while (selectedIndex < 0) {
          selectedIndex += entries.size();
        }
      }
    }

    display.fillScreen(BG_COLOR);

    int16_t headerHeight = screens::drawHeader();

    display.setFont(&Inconsolata_Bold10pt7b);

    display.setCursor(0, 20 + headerHeight);
    for(int i = 0; i < entries.size(); i++) {

      if(selectedIndex == i){
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(entries[i].name, display.getCursorX(), display.getCursorY(), &x1, &y1, &w, &h);

        display.fillRect(0, y1 - margin, 200, h + 2 * margin, FG_COLOR);
      }
      display.setTextColor(i == selectedIndex ? BG_COLOR : FG_COLOR);
      display.println(entries[i].name);
    }
    display.display(!initial);
    display.hibernate();
    initial = false;

    rtc::resetAlarm();
    esp_sleep_enable_ext0_wakeup(RTC_PIN, 0);
    esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_light_sleep_start();
  }
}
