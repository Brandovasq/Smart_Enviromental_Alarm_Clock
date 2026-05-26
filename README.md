# Smart Environmental Alarm Clock (ESP32)

## 1. Project Overview
This repository documents a finished **ESP32-based light-responsive alarm system** designed for an electrical engineering co-op portfolio. The project combines embedded firmware, sensor interfacing, real-time clock integration, basic signal interpretation, cloud-assisted data retrieval, and mechanical enclosure design.

The system uses:
- an ESP32 development board for control and connectivity
- a DS3231 RTC module for reliable timekeeping after startup
- an LCD for local status display
- a DHT sensor for indoor temperature/humidity monitoring
- an analog phototransistor circuit for ambient light sensing
- a passive buzzer for alarm output
- one momentary push button for display and user interaction

Wi-Fi is used only for **NTP time synchronization** at startup and **outside weather data retrieval**.

## 2. Features
- Displays time and date from DS3231 RTC
- Displays indoor temperature and humidity (DHT sensor)
- Displays ambient light level from phototransistor ADC readings
- Displays outside weather data fetched over Wi-Fi
- Triggers buzzer in a defined morning window when light exceeds threshold
- Uses one button for display mode switching and sleep/wake control
- Integrates embedded firmware, electronics documentation, and enclosure CAD assets

## 3. Hardware Used
- ESP32 development board
- DS3231 RTC module
- Character/graphic LCD display (project-configured)
- DHT temperature/humidity sensor
- Phototransistor + resistor divider (analog light sensing)
- Passive buzzer
- Momentary push button
- Supporting wiring, breadboard/PCB interconnects, and power components

## 4. Wiring Summary
> Detailed net-level connections are documented in `hardware/` (KiCad files).

High-level signal mapping:
- **I2C bus**: ESP32 ↔ DS3231 RTC (and LCD if I2C backpack is used)
- **Digital GPIO**: ESP32 ↔ DHT data pin
- **Analog GPIO (ADC)**: ESP32 ↔ phototransistor divider output
- **Digital GPIO**: ESP32 ↔ passive buzzer control
- **Digital GPIO**: ESP32 ↔ momentary push button input (with pull-up/pull-down per schematic)

## 5. Software/Libraries
Typical firmware dependencies include:
- ESP32 Arduino core
- Wire (I2C)
- RTClib (or equivalent DS3231 library)
- DHT sensor library
- LCD driver library (based on selected display module)
- WiFi + HTTP client stack for weather API access
- NTP client/time sync utility during startup

Project source code should remain in `src/`.

## 6. Display Modes
The one-button interface supports multiple display views and display power behavior.

Example mode set:
1. Time/Date primary view
2. Indoor climate (temperature/humidity)
3. Ambient light status
4. Outside weather summary

Button behavior:
- short press: cycle display mode
- long press (or configured timeout logic): sleep/wake display

## 7. CAD Enclosure
The custom enclosure was designed in **Fusion 360** to support practical hardware integration:
- ESP32 board fitment
- LCD viewing window
- sensor openings/airflow considerations
- buzzer sound path
- button access and cable management

Store enclosure exports/screenshots in `cad/`.

## 8. Photos
Place build photos, wiring snapshots, and finished assembly images in:
- `docs/images/`

These images are useful for demonstrating hardware/software integration in portfolio reviews.

## 9. Future Improvements
- Add non-volatile settings storage (thresholds, alarm window, preferred mode)
- Add automatic brightness/contrast adaptation for display readability
- Improve light-trigger detection with filtering/hysteresis tuning
- Add battery backup/monitoring telemetry
- Add robust Wi-Fi reconnection and weather API fault handling
- Add optional PCB revision for compact integration

## 10. Safety/Privacy Note
Do **not** commit real Wi-Fi credentials, API keys, or private network details.

Use `src/config_example.h` as a template and keep your real `config.h` local only.

---

## Repository Structure

```text
.
├── README.md
├── src/
│   └── config_example.h
├── hardware/
├── cad/
└── docs/
    └── images/
```
