#include "testing.h"

#include <cstdint>
#include <string>
#include <iostream>

#include "constants.h"
#include "res/fonts/Inconsolata_Bold10pt7b.h"
#include "screens.h"

#include "e_ink.h"
using namespace e_ink;

#include <notifications.h>
#include <rtc.h>

screens::Screen testScreen(bool wakeFromSleep) {
  if(wakeFromSleep) {
    return screens::WATCHFACE;
  }

  auto notifications = Notification::all();
  std::cout << "Notifications: " << std::endl;
  for(const auto& notification : notifications) {
    std::cout << std::to_string(notification->id) << ", " << std::to_string(notification->appId) << ", " << notification->title << ", " << notification->text << std::endl;
  }

  // Notification::create(5, Notification::AppID::WHATSAPP, std::string("Test"), std::string("Text"), rtc::currentTime());

  return screens::TEST_SCREEN;
}
