#include "testing.h"

#include <cstdint>

#include "constants.h"
#include "res/fonts/Inconsolata_Bold10pt7b.h"
#include "e_ink.h"
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

screens::Screen testScreen(bool wakeFromSleep) {
  if(wakeFromSleep) {
    return screens::WATCHFACE;
  }

  display.fillScreen(BG_COLOR);
  display.setFont(&Inconsolata_Bold10pt7b);
  display.setTextColor(FG_COLOR);

  display.setCursor(0, 30);
  display.println("Testing...");

  display.display();
  display.hibernate();

  ble::updateTime(ble::REBOOT, 10'000'000);

  // ServerCallbacks callbacks;

  // BLEDevice::init("WatchyOS");
  // BLEServer *pServer = BLEDevice::createServer();
  // pServer->setCallbacks(&callbacks);
  // BLEService *pService = pServer->createService(SERVICE_UUID);
  // BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         // CHARACTERISTIC_UUID,
                                         // BLECharacteristic::PROPERTY_READ |
                                         // BLECharacteristic::PROPERTY_WRITE
                                       // );

  // pCharacteristic->setValue("Hello World says Neil");
  // pService->start();
  // // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  // BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  // pAdvertising->addServiceUUID(SERVICE_UUID);
  // pAdvertising->setScanResponse(true);
  // pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  // pAdvertising->setMinPreferred(0x12);
  // pAdvertising->setMinInterval(0x20);
  // pAdvertising->setMaxInterval(0x40);


  // BLEDevice::startAdvertising();
  // auto begin_time = esp_timer_get_time();

  // printf("Started advertising\n");

  // gpio_config_t pin_config;
  // pin_config.pin_bit_mask = BACK_BTN_MASK;
  // pin_config.mode = GPIO_MODE_INPUT;
  // pin_config.pull_up_en = GPIO_PULLUP_DISABLE;
  // pin_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  // pin_config.intr_type = GPIO_INTR_DISABLE;

  // gpio_config(&pin_config);

  // while(!gpio_get_level(BACK_BTN_GPIO) && !callbacks.disconnected) {
    // vTaskDelay(1);
  // }

  // auto now = esp_timer_get_time();

  // printf("Total BLE time: %lli\n", (now - begin_time) / 1000);

  // pServer->disconnect(pServer->getConnId());

  // BLEDevice::stopAdvertising();


  // BLEDevice::deinit();


  return screens::WATCHFACE;
}
