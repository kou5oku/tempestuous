#include <Arduino.h>         // Required for delay(), Serial, etc.
#include <WiFiManager.h>     // Captive portal WiFi config
#include <ArduinoJson.h>     // JSON parsing for Tempest API
#include <HTTPClient.h>      // Needed for HTTP requests
#include <LovyanGFX.hpp>     // LovyanGFX display framework
#include "lgfx_user_setup.h" // Custom pinout and TFT setup
#include <Ticker.h> // For debounce timing
#include <Wire.h>
#include "background2.h"
//#include "weather_icons.h"

// 📲 OpenWeatherMap API for London
const char* OPENWEATHER_API_KEY = "OPEN_WEATHER_API_KEY";
const char* OPENWEATHER_LONDON_URL = "http://api.openweathermap.org/data/2.5/weather?q=London,UK&units=imperial&appid=";

// 🧭 Track current screen state
int currentScreen = 0;  // 0 = San Diego (Tempest), 1 = London (OpenWeather)
bool shouldRedraw = true;
const uint32_t screenInterval = 30000;  // 30 seconds
unsigned long lastSwitchTime = 0;  

// 👈 Prep Screen
LGFX tft;
LGFX_Sprite touchSprite(&tft);  

// 🌤️ Tempest Station API URL (replace with another if gifting multiple)
const char* TEMPEST_API_URL = "https://swd.weatherflow.com/swd/rest/observations/station/170405";
const char* TEMPEST_API_KEY = "Tempest_API_KEY";

void fetchAndDisplayWeather();

// Wifi Signal Bar Setup
void showWiFiSignalBars(int strength) {
  const int totalBars = 5;
  const int barWidth = 20;
  const int barSpacing = 5;
  const int baseX = (240 - ((barWidth + barSpacing) * totalBars - barSpacing)) / 2;
  const int baseY = 60;

  tft.fillRect(0, baseY, 240, 40, TFT_SKYBLUE);  // Clear area

  for (int i = 0; i < totalBars; i++) {
    uint16_t color = (i < strength) ? TFT_GREEN : TFT_LIGHTGREY;
    int barHeight = (i + 1) * 8;
    int x = baseX + i * (barWidth + barSpacing);
    int y = baseY + (40 - barHeight);
    tft.fillRect(x, y, barWidth, barHeight, color);
  }
}

// Screen Title Helper Module
void drawScreenTitle(const char* title) {
  tft.setFont(&fonts::Font4);  // Big bold title font
  tft.fillRect(0, 0, 240, 50, TFT_SKYBLUE);  // 🧱 Taller title bar
  int16_t titleX = (240 - tft.textWidth(title)) / 2;
  tft.setTextColor(TFT_WHITE);
  tft.drawString(title, titleX, 22);  // 🪂 Drop it down a bit
}


// Wind Direction Helper Module
String windDirFromDegrees(float deg) {
  const char* directions[] = {
    "N", "NNE", "NE", "ENE",
    "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW",
    "W", "WNW", "NW", "NNW"
  };
  int index = (int)((deg + 11.25) / 22.5);
  return String(directions[index % 16]);
}


//Setup the app
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("🌈 Booting up Tempest Display...");

  tft.init();
  tft.setRotation(0);  // Adjust as needed for screen orientation

  tft.fillScreen(TFT_SKYBLUE);
  tft.setTextColor(TFT_PURPLE);
  tft.setFont(&fonts::Font2);

  // 🎨 "Connecting WiFi..." splash
  tft.setFont(&fonts::Font4);
  tft.setTextColor(TFT_PURPLE);
  String msg = "Connecting WiFi...";
  int16_t msgX = (240 - tft.textWidth(msg)) / 2;
  int16_t msgY = 105;
  tft.drawString(msg, msgX, msgY);

  // ⚡ Animate WiFi bars
  for (int i = 0; i <= 5; i++) {
    showWiFiSignalBars(i);
    delay(400);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin();

  Serial.println("🌐 Trying saved WiFi credentials...");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {  // ~10s
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected to WiFi!");
    tft.fillScreen(TFT_SKYBLUE);
    tft.setTextColor(TFT_PURPLE);
    tft.setFont(&fonts::Font4);
    String successMsg = "WiFi Connected!";
    int16_t successX = (240 - tft.textWidth(successMsg)) / 2;
    tft.drawString(successMsg, successX, 60);
  } else {
    Serial.println("\n⚠️ Saved WiFi failed. Starting WiFiManager AP...");
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_RED);
    tft.setFont(&fonts::Font2);
    String failMsg = "WiFi Setup Mode";
    int16_t failX = (240 - tft.textWidth(failMsg)) / 2;
    tft.drawString(failMsg, failX, 60);

    WiFiManager wm;
    wm.setConfigPortalTimeout(300);  // 5 min timeout for safety

    if (!wm.autoConnect("Tempest-Setup")) {
      Serial.println("❌ WiFiManager failed or timed out. Restarting...");
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_RED);
      tft.drawString("WiFi Failed!", 10, 60);
      delay(3000);
      ESP.restart();
    } else {
      Serial.println("✅ Connected via WiFiManager!");
      tft.fillScreen(TFT_SKYBLUE);
      tft.setTextColor(TFT_PURPLE);
      tft.setFont(&fonts::Font4);
      String successMsg = "WiFi Connected!";
      int16_t successX = (240 - tft.textWidth(successMsg)) / 2;
      tft.drawString(successMsg, successX, 60);
    }
  }

  shouldRedraw = true;
  currentScreen = 0;
  fetchAndDisplayWeather();
  lastSwitchTime = millis();
  shouldRedraw = false;
}

void fetchAndDisplayLondonWeather() {
  String fullUrl = String(OPENWEATHER_LONDON_URL) + OPENWEATHER_API_KEY;

  HTTPClient http;
  http.begin(fullUrl);
  int httpCode = http.GET();

  tft.setFont(&fonts::Font4);  // Just for title

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("🌍 London Weather API Response:");
    Serial.println(payload);

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.println("❌ JSON Parse Failed!");
      tft.fillScreen(TFT_BLACK);
      tft.drawString("JSON Error!", 10, 20);
      return;
    }

    float temp_f = doc["main"]["temp"].as<float>();

    tft.fillScreen(TFT_WHITE);
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 240, background2);

    tft.setTextColor(TFT_NAVY);
    tft.setFont(&fonts::Font6);  // Big temp
    tft.setTextSize(2);
    String tempText = String(temp_f, 1) + " F";
    int16_t tempX = (240 - tft.textWidth(tempText)) / 2;
    int16_t tempY = 120 - (tft.fontHeight() / 2);
    tempX += 15;
    tempY += 20;
    tft.drawString(tempText, tempX, tempY);
    tft.setTextSize(1);
    drawScreenTitle("London");

  } else {
    Serial.println("❌ Failed to connect to OpenWeather API.");
    tft.fillScreen(TFT_BLACK);
    tft.drawString("API Error!", 10, 20);
  }

  http.end();
}

void fetchAndDisplayWeather() {
  HTTPClient http;
  http.begin(TEMPEST_API_URL);
  String bearer = "Bearer " + String(TEMPEST_API_KEY);
  http.addHeader("Authorization", bearer);
  int httpCode = http.GET();

  tft.setFont(&fonts::Font4);  // Just for title

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("📡 Weather API Response:");
    Serial.println(payload);

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.println("❌ JSON Parse Failed!");
      tft.fillScreen(TFT_BLACK);
      tft.drawString("JSON Error!", 10, 20);
      return;
    }

    float temp_c = doc["obs"][0]["air_temperature"].as<float>();
    float temp_f = (temp_c * 9.0 / 5.0) + 32.0;

    tft.fillScreen(TFT_WHITE);
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 240, background2);

    tft.setTextColor(TFT_NAVY);
    tft.setFont(&fonts::Font6);  // Big temp
    tft.setTextSize(2);
    String tempText = String(temp_f, 1) + " F";
    int16_t tempX = (240 - tft.textWidth(tempText)) / 2;
    int16_t tempY = 120 - (tft.fontHeight() / 2);
    tempX += 15;
    tempY += 20;
    tft.drawString(tempText, tempX, tempY);
    tft.setTextSize(1);
    drawScreenTitle("San Diego");

  } else {
    Serial.println("❌ Failed to connect to API.");
    tft.fillScreen(TFT_BLACK);
    tft.drawString("API Error!", 10, 20);
  }

  http.end();
}



void loop() {
  uint32_t now = millis();

  // ⏱ Auto-switch every 10s
  if (now - lastSwitchTime > screenInterval) {
    currentScreen = (currentScreen + 1) % 2;
    shouldRedraw = true;
    lastSwitchTime = now;
  }

  if (shouldRedraw) {
    if (currentScreen == 0) {
      fetchAndDisplayWeather();  // 🌞 San Diego
    } else {
      fetchAndDisplayLondonWeather();  // 🌧️ London
    }
    shouldRedraw = false;
  }

  delay(20);  // 🌿 Chill a bit
}
