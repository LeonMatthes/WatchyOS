#include "screens_menu.h"
#include "rtc.h"

using namespace screens;

#include <array>
#include <cstdint>

#include "constants.h"
#include <event_queue.h>
#include "res/fonts/Inconsolata_Bold10pt7b.h"
#include "e_ink.h"
using namespace e_ink;

struct MenuEntry {
  Screen screen;
  const char* name;
};

const std::array<MenuEntry, 4> entries = {{
  {
    .screen = TIMER,
    .name = "Timer"
  },
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

#include <vibration_motor.h>

screens::Screen screensMenu(bool wakeFromSleep) {
  int selectedIndex = 0;
  const int margin = 2;

  bool initial = true;
  while(true) {
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

    // wait for a button event to occur
    bool btn_pressed = false;
    do {
      auto event = event_queue::queue.receive(portMAX_DELAY);
      if(event) {
        switch (*event) {
          case event_queue::Event::MENU_BUTTON: 
            return entries[selectedIndex].screen;
          case event_queue::Event::BACK_BUTTON:
            return WATCHFACE;
          case event_queue::Event::DOWN_BUTTON:
            selectedIndex++;
            selectedIndex %= entries.size();
            btn_pressed = true;
            break;
          case event_queue::Event::UP_BUTTON:
            selectedIndex--;
            while(selectedIndex < 0) {
              selectedIndex += entries.size();
            }
            btn_pressed = true;
            break;
          default:
            break;
        }
      }
    }
    while(!btn_pressed);

    initial = false;
  }
}
