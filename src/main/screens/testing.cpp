#include "testing.h"

#include <cstdint>
#include <string>

#include "constants.h"
#include "res/fonts/Inconsolata_Bold10pt7b.h"
#include "e_ink.h"
#include "screens.h"
using namespace e_ink;

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class ServerCallbacks : public BLEServerCallbacks {
  public:
    bool disconnected = false;

    virtual ~ServerCallbacks() {}
    virtual void onConnect(BLEServer* server) override {
      printf("Client connected\n");
    }
    virtual void onConnect(BLEServer* server, esp_ble_gatts_cb_param_t *param) override {
      printf("Client connected 2\n");
    }

    virtual void onDisconnect(BLEServer* server) override {
      printf("Client disconnected\n");
      disconnected = true;
    }
};

#include "ble/update_time.h"

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
