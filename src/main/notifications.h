#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <array>

#include <TimeLib.h>


class Notification {
  public:
    enum AppID {
      WHATSAPP = 0x00,
      EMAIL=0x01,
      UNKNOWN = 0xFF
    };

    struct RTCNotification {
      uint8_t id;
      enum AppID appId;
      const char* title;
      const char* text;
      TimeElements when;
    };

    Notification(uint8_t id, enum AppID appId, const std::string& title, const std::string& text, const TimeElements& when);
    Notification(const RTCNotification& rtcNotification);

    uint8_t id;
    enum AppID appId;

    std::string title;
    std::string text;

    TimeElements when;

    const unsigned char* icon20x20() const;

    void remove();

    static std::vector<std::shared_ptr<const Notification>> all();
    static void storeInRTC();
    // inserting a notification with the same id will replace the existing one
    static std::shared_ptr<const Notification> create(uint8_t id, enum AppID appId, const std::string& title, const std::string& text, const TimeElements& when);
    static void removeAll();
    static void remove(uint8_t id);

    bool operator<(const Notification& other) const;

  private:
    static std::optional<std::vector<std::shared_ptr<const Notification>>> loadedNotifications;
    static std::mutex mutex;
};
