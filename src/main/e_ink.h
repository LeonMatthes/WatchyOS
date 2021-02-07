#pragma once

#include <GxEPD2_BW.h>

#define BG_COLOR GxEPD_BLACK
#define FG_COLOR GxEPD_WHITE

namespace e_ink {
  extern GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display;

}
