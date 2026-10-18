#pragma once
// Copy this file to secrets.h (which is git-ignored) and fill in your details.

// Home Wi-Fi the car joins when it's in range.
#define WIFI_SSID     "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"

// Access point the car creates when the home network isn't available.
// WPA2 needs at least 8 characters.
#define AP_SSID       "ESP32-CAM Robot"
#define AP_PASSWORD   "change-me-please"

// Wireless firmware updates; must match ota_password in secrets.ini.
#define OTA_PASSWORD  "change-me-too"
