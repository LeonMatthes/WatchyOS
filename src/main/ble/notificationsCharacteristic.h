#pragma once

#include <NimBLEServer.h>

class NotificationCharacteristic : public NimBLECharacteristicCallbacks {
  public: 
    virtual ~NotificationCharacteristic() {}

    NimBLECharacteristic* createNimbleCharacteristic(NimBLEService* service);

    virtual void onWrite(NimBLECharacteristic* characteristic) override;

    enum NotificationCommand {
      CREATE = 0x00,
      REMOVE = 0x01,
      REMOVE_ALL = 0x02
    };
};

