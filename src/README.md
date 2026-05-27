# Firmware

The ESP32 Arduino sketch is in:

```text
src/light_responsive_alarm/light_responsive_alarm.ino
```

## Setup

1. Open the sketch folder in the Arduino IDE.
2. Copy `config_example.h` to `config.h`.
3. Add your local Wi-Fi name and password to `config.h`.
4. Select an ESP32 development board target.
5. Install the required Arduino libraries.
6. Build and upload.

## Main Libraries

- WiFi
- HTTPClient
- ArduinoJson
- Wire
- LiquidCrystal_I2C
- RTClib
- DHT
- time.h

`config.h` is ignored by git so real Wi-Fi credentials stay local.
