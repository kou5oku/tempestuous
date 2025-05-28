#include <Arduino.h>         // Required for delay(), Serial, etc.
#include <WiFiManager.h>     // Captive portal WiFi config
#include <ArduinoJson.h>     // JSON parsing for Tempest API
#include <HTTPClient.h>      // Needed for HTTP requests
#include <LovyanGFX.hpp>     // LovyanGFX display framework
#include "lgfx_user_setup.h" // Custom pinout and TFT setup

LGFX tft;

// 🌤️ Tempest Station API URL (replace with another if gifting multiple)
const char* TEMPEST_API_URL = "https://swd.weatherflow.com/swd/rest/observations/station/170405";

void fetchAndDisplayWeather();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("🌈 Booting up Tempest Display...");

  tft.init();
  tft.setRotation(0);  // Adjust as needed for screen orientation
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.setFont(&fonts::Font2);  // Make it larger and better aligned
  String msg = "Connecting WiFi...";
  int16_t msgX = (240 - tft.textWidth(msg)) / 2;
  tft.drawString(msg, msgX, 30);  // Adjust Y as needed
 

  // 📶 Auto-start WiFiManager portal if no credentials stored
  WiFiManager wm;
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
  tft.fillScreen(TFT_BLACK);
    // On WiFi success
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.setFont(&fonts::Font2);
  String successMsg = "WiFi Connected!";
  int16_t successX = (240 - tft.textWidth(successMsg)) / 2;
  tft.drawString(successMsg, successX, 60);

  // 🌦️ Pull current weather and display
  fetchAndDisplayWeather();
}
void fetchAndDisplayWeather() {
  HTTPClient http;
  http.begin(TEMPEST_API_URL);
  int httpCode = http.GET();

  tft.setFont(&fonts::Font2); // 🔠 Larger font

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

    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_DARKCYAN);
    String tempText = "Temp: " + String(temp_c) + "°C";
    int16_t x1 = (240 - tft.textWidth(tempText)) / 2;
    tft.drawString(tempText, x1, 10);

    String windText = "Wind: " + String(wind_kph) + " km/h";
    int16_t x2 = (240 - tft.textWidth(windText)) / 2;
    tft.drawString(windText, x2, 40);

    String pressureText = "Pressure: " + String(pressure) + " hPa";
    int16_t x3 = (240 - tft.textWidth(pressureText)) / 2;
    tft.drawString(pressureText, x3, 70);

    // ➖ Divider line
    tft.drawLine(10, 105, 230, 105, TFT_WHITE);

    // 🔊 BRIGHT ALERT!
    tft.setTextColor(TFT_RED);
    tft.setFont(&fonts::Font4);  // or Font6 for MAXYELL
    String msg = "OK MATT, I NEED";
    int16_t msgX = (240 - tft.textWidth(msg)) / 2;
    tft.drawString(msg, msgX, 115);

    String msg2 = "YOUR API KEY!";
    int16_t msg2X = (240 - tft.textWidth(msg2)) / 2;
    tft.drawString(msg2, msg2X, 145);
  } else {
    Serial.println("❌ Failed to connect to API.");
    tft.fillScreen(TFT_BLACK);
    tft.drawString("API Error!", 10, 20);
  }

  http.end();
}

void loop() {
  // 🔁 Optional: refresh every few minutes if you add battery/USB power
}
