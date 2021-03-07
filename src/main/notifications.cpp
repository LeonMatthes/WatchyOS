#include "notifications.h"

#include <array>
#include <iterator>
#include <vector>
#include <cstring>
#include <algorithm>

#include <esp_log.h>
#include <esp_attr.h>

static const char* TAG = "Notifications";

// alloc 1KB of RTC memory for Notification texts
RTC_DATA_ATTR std::array<char, 1000> rtcNotificationsText = {};
RTC_DATA_ATTR std::array<std::optional<struct Notification::RTCNotification>, 20> rtcNotifications = {};

using namespace std;

optional<vector<shared_ptr<const Notification>>> Notification::loadedNotifications;
mutex Notification::mutex;


Notification::Notification(uint8_t _id, enum AppID _appId, const std::string& _title, const std::string& _text, const TimeElements& _when) :
  id(_id),
  appId(_appId),
  title(_title),
  text(_text),
  when(_when)
{ }

Notification::Notification(const Notification::RTCNotification& rtcNotification) :
  id(rtcNotification.id),
  appId(rtcNotification.appId),
  title(rtcNotification.title),
  text(rtcNotification.text),
  when(rtcNotification.when)
{ }

#include <res/icons.h>

const unsigned char* Notification::icon20x20() const {
  switch(appId) {
    case WHATSAPP:
      return icon_whatsapp;
    case UNKNOWN:
      return icon_unknown_notification;
  }
  return icon_unknown_notification;
}

std::vector<shared_ptr<const Notification>> loadFromRTC() {
  vector<shared_ptr<const Notification>> notifications;
  for(const auto& rtcNotification : rtcNotifications) {
    if(rtcNotification) {
      notifications.emplace_back(std::make_shared<Notification>(*rtcNotification));
    }
  }
  return std::move(notifications);
}

void Notification::storeInRTC() {
  auto guard = lock_guard(Notification::mutex);

  if(!loadedNotifications) {
    // no notifications to store
    return;
  }

  ESP_LOGI(TAG, "Storing %i notifications in RTC", loadedNotifications->size());
  auto charIndex = 0;
  auto rtcNotificationsIndex = 0;
  for (const auto& notification : *loadedNotifications) {
    // +1 for null bytes
    if(rtcNotificationsText.size() <= charIndex + notification->title.size() + 1 + notification->text.size() + 1) {
      ESP_LOGW(TAG, "Notification is too large for rtc storage!");
      continue;
    }
    if(rtcNotificationsIndex >= rtcNotifications.size()) {
      ESP_LOGW(TAG, "Too many Notifications for RTC storage!");
      break;
    }

    char* titlePtr = &rtcNotificationsText[charIndex];
    strcpy(titlePtr, notification->title.c_str());
    charIndex += notification->title.size() + 1;

    char* textPtr = &rtcNotificationsText[charIndex];
    strcpy(textPtr, notification->text.c_str());
    charIndex += notification->text.size() + 1;

    rtcNotifications[rtcNotificationsIndex++] = {
      .id = notification->id,
      .appId = notification->appId,
      .title = titlePtr,
      .text = textPtr,
      .when = notification->when
    };
  }

  // clear out all remaining notifications
  for(; rtcNotificationsIndex < rtcNotifications.size(); rtcNotificationsIndex++) {
    rtcNotifications[rtcNotificationsIndex] = {};
  }
}

vector<shared_ptr<const Notification>> Notification::all() {
  auto guard = lock_guard(Notification::mutex);

  if(!loadedNotifications) {
    loadedNotifications = loadFromRTC();
  }
  return *loadedNotifications;
}

shared_ptr<const Notification> Notification::create(uint8_t id, enum AppID appId, const std::string& title, const std::string& text, const TimeElements& when) {
  auto guard = lock_guard(Notification::mutex);

  if(!loadedNotifications) {
    loadedNotifications = loadFromRTC();
  }

  auto existing = find_if(
      loadedNotifications->begin(),
      loadedNotifications->end(),
      [&](shared_ptr<const Notification> notification){ return notification->id == id; });

  if(existing != loadedNotifications->end()) {
    *existing = make_shared<Notification>(id, appId, title, text, when);
    return *existing;
  }
  else {
    loadedNotifications->emplace_back(make_shared<Notification>(id, appId, title, text, when));
    return loadedNotifications->back();
  }
}

void Notification::remove(uint8_t id) {
  auto guard = lock_guard(Notification::mutex);
  if(!loadedNotifications) {
    loadedNotifications = loadFromRTC();
  }

  loadedNotifications->erase(remove_if(
        loadedNotifications->begin(),
        loadedNotifications->end(),
        [&](shared_ptr<const Notification> notification){ return notification->id == id; }));
}

void Notification::remove() {
  Notification::remove(this->id);
}

void Notification::removeAll() {
  auto guard = lock_guard(Notification::mutex);
  loadedNotifications = vector<shared_ptr<const Notification>>();
}
