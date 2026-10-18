/*
  ESP32-CAM Robot Car

  Wi-Fi and OTA setup. The camera lives in camera.cpp, the web interface and
  its WebSocket control channel in web_server.cpp, the motor driver in car.cpp.

  Hardware and pin map from the DroneBot Workshop ESP32-CAM robot car (2021):
  https://dronebotworkshop.com/esp32cam-robot-car/
*/

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "camera.h"
#include "car.h"
#include "web_server.h"
#include "secrets.h"

static const int MIN_RSSI = -80;
static const char *HOSTNAME = "robot";   // http://robot.local

static void connect_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.disconnect();
  delay(100);

  bool in_range = false;
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == WIFI_SSID) {
      Serial.printf("Found '%s' at %d dBm\n", WIFI_SSID, WiFi.RSSI(i));
      in_range = WiFi.RSSI(i) > MIN_RSSI;
      break;
    }
  }
  WiFi.scanDelete();

  if (in_range) {
    Serial.print("Connecting");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
      delay(500);
      Serial.print('.');
    }
    Serial.println();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Connected to '%s', IP %s\n", WIFI_SSID, WiFi.localIP().toString().c_str());
  } else {
    Serial.println("Home Wi-Fi unavailable, starting access point");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.printf("Access point '%s', IP %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  }

  // Modem sleep adds 100ms+ to every packet, which makes driving feel laggy.
  WiFi.setSleep(false);

  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("Also reachable at http://%s.local\n", HOSTNAME);
  }
}

static void start_ota() {
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setMdnsEnabled(false);   // already advertised by connect_wifi()
  ArduinoOTA.onStart([]() {
    car_stop();
    camera_stop();   // frees the radio and CPU for the upload
    Serial.println("OTA update starting");
  });
  ArduinoOTA.onError([](ota_error_t err) { Serial.printf("OTA error %u\n", err); });
  ArduinoOTA.begin();
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // motors starting would trip it

  Serial.begin(115200);
  Serial.println();

  car_setup();             // first, so the motor pins aren't left floating
  camera_start(0);         // keep going without video so the car still drives
  connect_wifi();
  start_ota();
  web_server_start();

  for (int i = 0; i < 3; i++) {
    car_light(10);
    delay(60);
    car_light(0);
    delay(60);
  }
}

void loop() {
  ArduinoOTA.handle();
  car_loop();
  delay(5);
}
