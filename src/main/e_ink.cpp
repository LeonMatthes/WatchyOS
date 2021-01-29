#include "e_ink.h"

#include "constants.h"

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> e_ink::display(GxEPD2_154_D67(CS, DC, RESET, BUSY_PIN));
