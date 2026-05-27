#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <DHT.h>
#include "time.h"

// ---------------- Wi-Fi ----------------
// Copy config_example.h to config.h and fill in your local Wi-Fi values.
// config.h is intentionally ignored by git.
#if __has_include("config.h")
#include "config.h"
#else
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
#endif

// Eastern time zone with daylight saving
const char* tzInfo = "EST5EDT,M3.2.0/2,M11.1.0/2";
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.google.com";
const char* ntpServer3 = "time.nist.gov";

// ---------------- Weather location ----------------
// Cincinnati / University of Cincinnati area.
const float LATITUDE = 39.1329;
const float LONGITUDE = -84.5150;
const char* CITY_NAME = "Cincinnati";

// ---------------- Hardware ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

const int dhtPin = 27;
#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);

const int lightPin = 35;
const int buzzerPin = 25;
const int buttonPin = 26;

// ---------------- Alarm settings ----------------
const int startHour = 8;
const int endHour = 9;

int lightThreshold = 300;
int lightMax = 1000;

bool alarmTriggeredToday = false;
bool alarmActive = false;
int lastDateKey = -1;

// ---------------- Display modes ----------------
int displayMode = 0;
const int totalModes = 3;

bool displaySleeping = false;

const char* daysOfWeek[] = {"Su", "Mo", "Tu", "We", "Thu", "Fr", "Sa"};

// ---------------- Button handling ----------------
bool lastButtonReading = HIGH;
bool buttonStableState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long buttonPressedTime = 0;
bool longPressHandled = false;

const unsigned long debounceDelay = 40;
const unsigned long longPressTime = 900;

// ---------------- Sensor values ----------------
float indoorTempF = NAN;
float indoorHumidity = NAN;
int lightRaw = 0;

unsigned long lastDHTRead = 0;
const unsigned long dhtReadInterval = 2000;

// ---------------- Weather values ----------------
float outsideTempF = NAN;
int outsideWeatherCode = -1;
String outsideCondition = "No data";

unsigned long lastWeatherFetch = 0;
const unsigned long weatherFetchInterval = 30UL * 60UL * 1000UL;

// ---------------- Buzzer timing ----------------
unsigned long lastBeepTime = 0;
int beepCount = 0;
const int maxBeeps = 12;

// ---------------- Helper functions ----------------

void lcdPrintLine(int row, const char* text) {
  lcd.setCursor(0, row);
  lcd.print("                ");
  lcd.setCursor(0, row);
  lcd.print(text);
}

String weatherDescription(int code) {
  if (code == 0) return "Clear";
  if (code >= 1 && code <= 3) return "Cloudy";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Showers";
  if (code >= 95 && code <= 99) return "Storm";
  return "Weather";
}

bool connectWiFi(unsigned long timeoutMs = 12000) {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < timeoutMs) {
    delay(300);
    Serial.print(".");
  }

  Serial.println();

  return WiFi.status() == WL_CONNECTED;
}

bool syncRTCFromNTP() {
  if (!connectWiFi()) {
    Serial.println("WiFi failed. RTC not updated.");
    return false;
  }

  configTzTime(tzInfo, ntpServer1, ntpServer2, ntpServer3);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 20000)) {
    Serial.println("NTP time failed. RTC not updated.");
    return false;
  }

  rtc.adjust(DateTime(
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  ));

  Serial.println("RTC updated from NTP.");
  return true;
}

bool fetchOutsideWeather() {
  if (!connectWiFi()) {
    Serial.println("Weather fetch failed: no WiFi.");
    return false;
  }

  String url = "http://api.open-meteo.com/v1/forecast?latitude=";
  url += String(LATITUDE, 4);
  url += "&longitude=";
  url += String(LONGITUDE, 4);
  url += "&current=temperature_2m,weather_code";
  url += "&temperature_unit=fahrenheit";
  url += "&timezone=America%2FNew_York";

  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.print("Weather HTTP error: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("Weather JSON error: ");
    Serial.println(error.c_str());
    return false;
  }

  outsideTempF = doc["current"]["temperature_2m"].as<float>();
  outsideWeatherCode = doc["current"]["weather_code"].as<int>();
  outsideCondition = weatherDescription(outsideWeatherCode);

  Serial.print("Outside temp: ");
  Serial.print(outsideTempF);
  Serial.print(" F, condition: ");
  Serial.println(outsideCondition);

  return true;
}

int readLightAveraged() {
  long sum = 0;
  const int samples = 10;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(lightPin);
    delay(2);
  }

  return sum / samples;
}

void updateSensors() {
  lightRaw = readLightAveraged();

  if (millis() - lastDHTRead >= dhtReadInterval) {
    float newTemp = dht.readTemperature(true);
    float newHumidity = dht.readHumidity();

    if (!isnan(newTemp)) {
      indoorTempF = newTemp;
    }

    if (!isnan(newHumidity)) {
      indoorHumidity = newHumidity;
    }

    lastDHTRead = millis();
  }
}

bool isMorningWindow(int hour24) {
  return (hour24 >= startHour && hour24 < endHour);
}

void stopBuzzer() {
  noTone(buzzerPin);
}

void dismissAlarmForToday() {
  stopBuzzer();
  alarmActive = false;
  alarmTriggeredToday = true;
  beepCount = 0;
}

void runLightAlarm(int lightValue) {
  if (alarmTriggeredToday) {
    stopBuzzer();
    alarmActive = false;
    return;
  }

  if (lightValue < lightThreshold) {
    stopBuzzer();
    alarmActive = false;
    beepCount = 0;
    return;
  }

  alarmActive = true;

  int clampedLight = constrain(lightValue, lightThreshold, lightMax);

  int pitch = map(clampedLight, lightThreshold, lightMax, 500, 3500);
  int beepDelay = map(clampedLight, lightThreshold, lightMax, 300, 50);

  if (millis() - lastBeepTime >= (unsigned long)beepDelay) {
    tone(buzzerPin, pitch, 70);
    lastBeepTime = millis();
    beepCount++;

    if (beepCount >= maxBeeps) {
      dismissAlarmForToday();
    }
  }
}

void sleepDisplay() {
  displaySleeping = true;
  lcd.noBacklight();
}

void wakeDisplay() {
  displaySleeping = false;
  lcd.backlight();
}

void nextDisplayMode() {
  displayMode++;
  if (displayMode >= totalModes) {
    displayMode = 0;
  }
}

void handleButton() {
  bool reading = digitalRead(buttonPin);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
    lastButtonReading = reading;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonStableState) {
      buttonStableState = reading;

      if (buttonStableState == LOW) {
        buttonPressedTime = millis();
        longPressHandled = false;

        if (alarmActive) {
          dismissAlarmForToday();
          longPressHandled = true;
        }

        if (displaySleeping) {
          wakeDisplay();
          longPressHandled = true;
        }
      }
      else {
        unsigned long pressDuration = millis() - buttonPressedTime;

        if (!longPressHandled && pressDuration < longPressTime) {
          nextDisplayMode();
        }
      }
    }
  }

  if (buttonStableState == LOW && !longPressHandled && !displaySleeping) {
    if (millis() - buttonPressedTime >= longPressTime) {
      sleepDisplay();
      longPressHandled = true;
    }
  }
}

void updateLCD(DateTime now) {
  if (displaySleeping) {
    return;
  }

  int hour24 = now.hour();
  int minute = now.minute();

  int displayHour = hour24;
  const char* period = "AM";

  if (displayHour >= 12) {
    period = "PM";
  }

  if (displayHour == 0) {
    displayHour = 12;
  } else if (displayHour > 12) {
    displayHour -= 12;
  }

  char line1[17];
  char line2[17];

  if (displayMode == 0) {
    // Example when it fits: Sun 5/17 11:35AM
    // If the date is too long, it removes the space after the day: Wed12/31 11:59PM
    int charsNeeded = snprintf(line1, sizeof(line1), "%s %d/%d %d:%02d%s",
                               daysOfWeek[now.dayOfTheWeek()],
                               now.month(),
                               now.day(),
                               displayHour,
                               minute,
                               period);

    if (charsNeeded > 16) {
      snprintf(line1, sizeof(line1), "%s%d/%d %d:%02d%s",
               daysOfWeek[now.dayOfTheWeek()],
               now.month(),
               now.day(),
               displayHour,
               minute,
               period);
    }

    if (isnan(indoorTempF) || isnan(indoorHumidity)) {
      snprintf(line2, sizeof(line2), "Indoor waiting");
    } else {
      snprintf(line2, sizeof(line2), "T:%4.1fF H:%2.0f%%", indoorTempF, indoorHumidity);
    }
  }

  else if (displayMode == 1) {
    snprintf(line1, sizeof(line1), "Light:%4d", lightRaw);

    if (alarmTriggeredToday) {
      snprintf(line2, sizeof(line2), "Alarm:Done %d-%d", startHour, endHour);
    } else {
      snprintf(line2, sizeof(line2), "Alarm:On   %d-%d", startHour, endHour);
    }
  }

  else if (displayMode == 2) {
    snprintf(line1, sizeof(line1), "%s WX", CITY_NAME);

    if (isnan(outsideTempF)) {
      snprintf(line2, sizeof(line2), "Waiting...");
    } else {
      snprintf(line2, sizeof(line2), "%4.1fF %s", outsideTempF, outsideCondition.c_str());
    }
  }

  lcdPrintLine(0, line1);
  lcdPrintLine(1, line2);
}

// ---------------- Setup ----------------

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);

  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcdPrintLine(0, "Starting...");
  lcdPrintLine(1, "ESP32 Alarm");

  dht.begin();

  if (!rtc.begin()) {
    lcdPrintLine(0, "RTC not found");
    lcdPrintLine(1, "Check wiring");
    Serial.println("RTC not found.");
    delay(2500);
  } else {
    lcdPrintLine(0, "RTC found");
    lcdPrintLine(1, "Syncing time...");
    delay(1000);

    syncRTCFromNTP();
  }

  lcdPrintLine(0, "Getting weather");
  lcdPrintLine(1, "Please wait...");
  fetchOutsideWeather();
  lastWeatherFetch = millis();

  lcd.clear();
}

// ---------------- Main loop ----------------

void loop() {
  handleButton();
  updateSensors();

  DateTime now = rtc.now();

  int currentDateKey = now.year() * 10000 + now.month() * 100 + now.day();

  if (currentDateKey != lastDateKey) {
    alarmTriggeredToday = false;
    alarmActive = false;
    lastDateKey = currentDateKey;
  }

  if (isMorningWindow(now.hour())) {
    runLightAlarm(lightRaw);
  } else {
    stopBuzzer();
    alarmActive = false;
    beepCount = 0;
  }

  if (millis() - lastWeatherFetch >= weatherFetchInterval) {
    fetchOutsideWeather();
    lastWeatherFetch = millis();
  }

  static unsigned long lastLCDUpdate = 0;
  if (millis() - lastLCDUpdate >= 500) {
    updateLCD(now);
    lastLCDUpdate = millis();
  }

  // Debugging
  static unsigned long lastSerialPrint = 0;
  if (millis() - lastSerialPrint >= 2000) {
    Serial.print("Time: ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());

    Serial.print(" | Indoor F: ");
    Serial.print(indoorTempF);

    Serial.print(" | Humidity: ");
    Serial.print(indoorHumidity);

    Serial.print(" | Light: ");
    Serial.print(lightRaw);

    Serial.print(" | Mode: ");
    Serial.print(displayMode);

    Serial.print(" | Sleeping: ");
    Serial.print(displaySleeping ? "YES" : "NO");

    Serial.print(" | Outside F: ");
    Serial.println(outsideTempF);

    lastSerialPrint = millis();
  }
}
