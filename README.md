# WatchyOS

An easy operating system for [Watchy](https://github.com/sqfmi/Watchy).

To get started, please install the [ESP IDF](https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/get-started/index.html) (version 4.2)

## Goals:
- The basics
	- Show the time (updated every minute)
	- Show the date
	- Step counter
	- Display battery (if low)
- Connection to phone:
	- Show notifications
	- Dismiss notifications on phone
	- Music control
- Sleep mode to save battery
- Other
	- Timer/Stopwatch
- Stretch goals
	- Shortcut Tapping

### GUI Mockup
![GUI Mockup](https://github.com/LeonMatthes/WatchyOS/raw/main/res/gui-mockup.jpg)

## Notes
WatchyOS is currently using a custom fork of [ESP-Arduino](https://github.com/summivox/arduino-esp32) while we wait for ESP-Arduino to support ESP-IDF v4.2 officially

This means the outdated `make menuconfig` and `make flash monitor` must be used instead of the newer `idf.py`
