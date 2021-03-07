#include "update_time.h"

#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEServer.h>
#include <TimeLib.h>

#include <cstring>

#include "esp_log.h"

#include "../rtc.h"
#include "notificationsCharacteristic.h"

#define WATCHYOS_SERVICE_UUID        "f9ce43b7-d389-4add-adf7-82811c462ca1"
#define TIME_CHARACTERISTIC_UUID "e7e3232e-88c0-452f-abd1-003cc2ec24d3"
#define NOTIFICATIONS_CHARACTERISTIC_UUID "0e207741-1657-4e87-9415-ca2a67af12e5"
#define STATE_CHARACTERISTIC_UUID "54ea5218-bcc6-4870-baa9-06f25ab86b32"

const char* TAG = "BLE";

static RTC_DATA_ATTR uint8_t stateID = 0; // used to make sure Watchy & Phone don't get desynchronized

TimeElements ble::timeFromBytes(const char* data) {
  return TimeElements {
    // our incoming data is reverse from the tmElements declaration
    .Second = static_cast<uint8_t>(data[6]),
      .Minute = static_cast<uint8_t>(data[5]),
      .Hour = static_cast<uint8_t>(data[4]),
      .Wday = static_cast<uint8_t>(data[3]), // day of week, 1-7, sunday is 1
      .Day = static_cast<uint8_t>(data[2]),
      .Month = static_cast<uint8_t>(data[1]),
      .Year = static_cast<uint8_t>(data[0]) // offset from 1970
  };
}

class TimeCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  public:
    virtual ~TimeCharacteristicCallbacks() {}

    virtual void onWrite(NimBLECharacteristic* characteristic) override {
      std::string value = characteristic->getValue();
      if(7 == value.size()){
        rtc::setTime(ble::timeFromBytes(value.data()));
        rtc::initialized = true;
        ESP_LOGI(TAG, "Successfully updated time from Phone");
      }
      else {
        ESP_LOGW(TAG, "Received invalid new time of %i bytes", value.size());
      }
    }
};

class StateCharacteristicCallbaks : public NimBLECharacteristicCallbacks {
  BLEServer* m_server;
  public:
    StateCharacteristicCallbaks(BLEServer* server) : m_server{server} {}

    virtual ~StateCharacteristicCallbaks() {}

    virtual void onWrite(NimBLECharacteristic* characteristic, ble_gap_conn_desc* desc) override {
      std::string value = characteristic->getValue();
      if(2 == value.size()){
        stateID = value[1];
        ESP_LOGI(TAG, "Successfully updated state id from Phone");
        // indicates disconnection attempt by the phone
        // it is much faster for everyone involved if Watchy initiates the disconnect,
        // instead of the Android phone!
        if(value[0] == ble::DISCONNECT) {
          m_server->disconnect(desc->conn_handle);
        }
      }
      else {
        ESP_LOGW(TAG, "Received invalid new state of %i bytes", value.size());
      }
    }

    virtual void onRead(NimBLECharacteristic* characteristic) override {
      ESP_LOGI(TAG, "Reading state characteristic");
    }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  public:
    bool connected = false;

    virtual ~ServerCallbacks() {}

    virtual void onConnect(NimBLEServer* server) override {
      ESP_LOGI(TAG, "Client connected");
      connected = true;
    }

    virtual void onDisconnect(NimBLEServer* server) override {
      ESP_LOGI(TAG, "Client disconnected");
      connected = false;
    }
};

bool ble::updateTime(State connectionState /*= FAST_UPDATE*/, int64_t timeout/* = 3'000'000*/) {
  ServerCallbacks callbacks;
  TimeCharacteristicCallbacks timeCB;
  NotificationCharacteristic notificationCB;

  NimBLEDevice::init("WatchyOS");
  NimBLEServer *pServer = BLEDevice::createServer();
  pServer->advertiseOnDisconnect(false);
  pServer->setCallbacks(&callbacks);
  NimBLEService *pService = pServer->createService(WATCHYOS_SERVICE_UUID);
  NimBLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         TIME_CHARACTERISTIC_UUID,
                                         NIMBLE_PROPERTY::WRITE
                                       );
  pCharacteristic->setCallbacks(&timeCB);

  notificationCB.createNimbleCharacteristic(pService);

  StateCharacteristicCallbaks stateCallback(pServer);
  uint8_t state[] = { connectionState, stateID };
  NimBLECharacteristic *stateCharacteristic = pService->createCharacteristic(
                                          STATE_CHARACTERISTIC_UUID,
                                          NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ
                                          );
  stateCharacteristic->setValue(state, 2);
  stateCharacteristic->setCallbacks(&stateCallback);

  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  NimBLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(WATCHYOS_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  // pAdvertising->setMinPreferred(0x12);
  pAdvertising->setMaxPreferred(0x24);
  pAdvertising->setMinInterval(0x20 /* equivalent to 20ms*/);
  pAdvertising->setMaxInterval(0x20);


  NimBLEDevice::startAdvertising();
  auto begin_time = esp_timer_get_time();

  ESP_LOGI(TAG, "Started advertising");

  // delay until disconnected or 10 secs elapse
  while(!callbacks.connected && esp_timer_get_time() - begin_time < timeout) {
    vTaskDelay(1);
  }
  // update failed
  if(!callbacks.connected) {
    ESP_LOGW(TAG, "BLE Operation timed out");
    return false;
  }

  NimBLEDevice::stopAdvertising();
  // Add a second timeout, to make sure we eventually disconnect
  while(callbacks.connected && esp_timer_get_time() - begin_time < timeout * 10) {
    vTaskDelay(1);
  }

  ESP_LOGI(TAG, "BLE operation took %llims", (esp_timer_get_time() - begin_time) / 1'000);

  NimBLEDevice::deinit();

  return true;
}
