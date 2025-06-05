#include <Arduino.h>         // Required for delay(), Serial, etc.
#include <WiFiManager.h>     // Captive portal WiFi config
#include <ArduinoJson.h>     // JSON parsing for Tempest API
#include <HTTPClient.h>      // Needed for HTTP requests
#include <LovyanGFX.hpp>     // LovyanGFX display framework
#include "lgfx_user_setup.h" // Custom pinout and TFT setup
#include <Ticker.h> // For debounce timing
#include <Wire.h>
#include "weather_icons.h"

// 📲 OpenWeatherMap API for London
const char* OPENWEATHER_API_KEY = "6fadd6bef59debaa26cad2f5fa53ff77";
const char* OPENWEATHER_LONDON_URL = "http://api.openweathermap.org/data/2.5/weather?q=London,UK&units=imperial&appid=";

// 🧭 Track current screen state
int currentScreen = 0;  // 0 = San Diego (Tempest), 1 = London (OpenWeather)
bool shouldRedraw = true;
const uint32_t screenInterval = 10000;  // 10 seconds
unsigned long lastSwitchTime = 0;  

// 👈 Prep Screen
LGFX tft;
LGFX_Sprite touchSprite(&tft);  

// 🌤️ Tempest Station API URL (replace with another if gifting multiple)
const char* TEMPEST_API_URL = "https://swd.weatherflow.com/swd/rest/observations/station/170405";
const char* TEMPEST_API_KEY = "API-KEY";

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


  // 🎨 Fancier "Connecting WiFi..." 
  tft.setFont(&fonts::Font4);  // 🌟 Bigger and more stylish
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

  // 📶 Auto-start WiFiManager portal if no credentials stored
  WiFiManager wm;
  // TO reset Wifi uncomment the below line.
  //wm.resetSettings();

  if (!wm.autoConnect("Tempest-Setup")) {
    Serial.println("❌ WiFi connect failed. Rebooting...");
    // If WiFi fails
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_RED);
    tft.setFont(&fonts::Font2);
    String failMsg = "WiFi Failed!";
    int16_t failX = (240 - tft.textWidth(failMsg)) / 2;
    tft.drawString(failMsg, failX, 60);
    delay(3000);
    ESP.restart();
  }

  Serial.println("✅ WiFi Connected!");
  shouldRedraw = true;  // 🚀 Trigger initial screen
  tft.fillScreen(TFT_SKYBLUE);
  tft.setTextColor(TFT_PURPLE);
  tft.setFont(&fonts::Font4);
  String successMsg = "WiFi Connected!";
  int16_t successX = (240 - tft.textWidth(successMsg)) / 2;
  tft.drawString(successMsg, successX, 60);
  currentScreen = 0;
  // 🌦️ Pull current weather and display
  fetchAndDisplayWeather();   // 🎬 Show San Diego first!
  lastSwitchTime = millis();  // ⏰ Start the clock AFTER setup
  shouldRedraw = false;  
  
}

void fetchAndDisplayLondonWeather() {
  String fullUrl = String(OPENWEATHER_LONDON_URL) + OPENWEATHER_API_KEY;

  HTTPClient http;
  http.begin(fullUrl);
  int httpCode = http.GET();

  tft.setFont(&fonts::Font4);

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

    float temp_f   = doc["main"]["temp"].as<float>();
    float wind_mph = doc["wind"]["speed"].as<float>();
    float wind_d  = doc["wind"]["deg"].as<float>();
    float pressure = doc["main"]["pressure"].as<float>();

    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLUE);
    tft.pushImage(0, 0, 240, 240, background);
    String tempText = String(temp_f, 1) + " F";
    tft.drawString(tempText, 135, 82);
    String windText = String(wind_mph, 1);
    String windDirText = windDirFromDegrees(wind_d);
    tft.drawString(windText, 135, 164);
    tft.setFont(&fonts::Font2);
    tft.setTextColor(TFT_BLACK);
    tft.drawString(windDirText, 175, 164);
    tft.setFont(&fonts::Font4);
    tft.setTextColor(TFT_BLUE);

    String pressureText = String(((pressure)* 0.145038),1) ;
    
    tft.drawString(pressureText, 130, 122);
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
  tft.setFont(&fonts::Font4); // 🔠 Larger font

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
    float temp_c   = doc["obs"][0]["air_temperature"].as<float>();
    float wind_kph = doc["obs"][0]["wind_avg"].as<float>() * 3.6;
    float pressure = doc["obs"][0]["sea_level_pressure"].as<float>();
    float temp_f = (temp_c * 9.0 / 5.0) + 32.0;
    float wind_d = doc["obs"][0]["wind_direction"].as<float>();

    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLUE);
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 240, background);
    String tempText = String(temp_f, 1) + " F";
    tft.drawString(tempText, 135, 82);
    String windText = String(wind_kph, 1);
    String winDirText = windDirFromDegrees(wind_d) ;
    tft.drawString(windText, 135, 164); // next line
    tft.setFont(&fonts::Font2);
    tft.setTextColor(TFT_BLACK);
    tft.drawString(winDirText, 175, 164); // next line
    tft.setFont(&fonts::Font4);
    tft.setTextColor(TFT_BLUE);
    String pressureText = String(((pressure)* 0.145038),1) ;
    tft.drawString(pressureText, 130, 122); // next line
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
