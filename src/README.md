# src/

Place the ESP32 Arduino firmware source files for the alarm clock in this directory.

Recommended pattern:
- main sketch/source files
- module headers/sources (RTC, sensors, display, alarm logic)
- local `config.h` copied from `config_example.h` (do not commit `config.h`)
