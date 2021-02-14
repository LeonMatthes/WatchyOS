#include "testing.h"

#include <cstdint>
#include <string>

#include "constants.h"
#include "res/fonts/Inconsolata_Bold10pt7b.h"
#include "screens.h"

#include "e_ink.h"
using namespace e_ink;

screens::Screen testScreen(bool wakeFromSleep) {
  if(wakeFromSleep) {
    return screens::WATCHFACE;
  }

  display.fillScreen(BG_COLOR);
  int16_t headerHeight = screens::drawHeader();

  display.setFont(&Inconsolata_Bold10pt7b);
  display.setCursor(0, 20 + headerHeight);

  display.println("012345678901234567890");
  for(int i = 1; i < 20; i++) {
    display.println(std::to_string(i).c_str());
  }
  display.display();
  display.hibernate();


  return screens::TEST_SCREEN;
}
