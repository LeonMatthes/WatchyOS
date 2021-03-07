#include "notificationsCharacteristic.h"

#include "esp_log.h"

#include <notifications.h>
#include <ble/update_time.h>

static const char* TAG = "BLENotifications";

#define NOTIFICATIONS_CHARACTERISTIC_UUID "0e207741-1657-4e87-9415-ca2a67af12e5"


NimBLECharacteristic* NotificationCharacteristic::createNimbleCharacteristic(NimBLEService* service) {
  auto* characteristic = service->createCharacteristic(NOTIFICATIONS_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::WRITE);
  characteristic->setCallbacks(this);
  return characteristic;
}

using namespace std;

void createNotification(const string& value) {
  if(value.size() < 11) {
    ESP_LOGW(TAG, "Create command too short, expecting at least 11 bytes. Received: %i", value.size());
    return;
  }

  uint8_t id = value[1];
  enum Notification::AppID appId = static_cast<Notification::AppID>(value[2]);

  // consumes 7 bytes
  auto when = ble::timeFromBytes(&value.c_str()[3]);

  const char* c_str = value.c_str();
  const char* title = &c_str[10];
  size_t titleLength = strlen(title);

  if(titleLength + 10 >= value.size()) {
    ESP_LOGW(TAG, "Create command only contains a single string! - Title and text are required");
    return;
  }

  // text starts after title
  const char* text = &title[titleLength + 1];

  Notification::create(id, appId, title, text, when);
  ESP_LOGI(TAG, "Successfully created notification %X", id);
}

void NotificationCharacteristic::onWrite(NimBLECharacteristic* characteristic) {
  string value = characteristic->getValue();
  if(value.size() < 1) {
    ESP_LOGW(TAG, "Characteristic written with empty value");
    return;
  }

  switch(value[0]) {
    case REMOVE_ALL:
      Notification::removeAll();
      ESP_LOGI(TAG, "Deleted all notifications");
      break;
    case REMOVE:
      if(value.size() < 2) {
        ESP_LOGW(TAG, "Delete command missing Notification ID");
        break;
      }
      Notification::remove(value[1]);
      ESP_LOGI(TAG, "Deleted notification %X", value[1]);
      break;
    case CREATE:
      createNotification(value);
      break;
    default:
      ESP_LOGW(TAG, "Unknown command %X received", value[0]);
      break;
  }
}
