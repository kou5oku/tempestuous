#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LovyanGFX.hpp>
#include "GC9A01_XIAO_ESP32C3.hpp"  // From PrintMinion fork

GC9A01_XIAO_ESP32C3 lcd;

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* api_token = "Bearer YOUR_API_TOKEN";  // Include "Bearer " prefix
const char* api_url = "https://swd.weatherflow.com/swd/rest/observations/station/170405";

#define BG_COLOR TFT_BLACK
#define TEXT_COLOR TFT_WHITE

void connectWiFi();
void fetchAndDrawWeather();
void drawWeatherScreen(float temp, int humidity);

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(0);
  lcd.fillScreen(BG_COLOR);

  connectWiFi();
  delay(500);
  fetchAndDrawWeather();
}

void loop() {
  delay(60000); // Update every 60 sec
  fetchAndDrawWeather();
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  lcd.setTextSize(2);
  lcd.setTextColor(TEXT_COLOR);
  lcd.setCursor(20, 100);
  lcd.println("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }

  lcd.fillScreen(BG_COLOR);
  lcd.setCursor(20, 100);
  lcd.println("WiFi Connected!");
  delay(1000);
  lcd.fillScreen(BG_COLOR);
}

void fetchAndDrawWeather() {
  HTTPClient http;
  http.begin(api_url);
  http.addHeader("Authorization", api_token);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);

    float temp = doc["obs"][0]["air_temperature"];
    int humidity = doc["obs"][0]["humidity"];

    drawWeatherScreen(temp, humidity);
  } else {
    lcd.fillScreen(BG_COLOR);
    lcd.setCursor(10, 100);
    lcd.setTextSize(2);
    lcd.setTextColor(TFT_RED);
    lcd.println("API Error!");
  }

  http.end();
}

void drawWeatherScreen(float temp, int humidity) {
  lcd.fillScreen(BG_COLOR);
  lcd.setTextDatum(MC_DATUM);
  lcd.setTextColor(TEXT_COLOR);

  lcd.setTextSize(3);
  lcd.drawString("Current Weather", 120, 40);

  lcd.setTextSize(4);
  lcd.drawString(String(temp, 1) + "C", 120, 100);

  lcd.setTextSize(3);
  lcd.drawString(String(humidity) + "% RH", 120, 160);
}
