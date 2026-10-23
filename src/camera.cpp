#include <Arduino.h>
#include <Preferences.h>
#include <atomic>
#include "camera.h"

// AI-Thinker ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

static volatile bool running = false;
// Frames handed out and not yet returned. Atomic: the control and stream
// servers run in separate tasks.
static std::atomic<int> frames_out{0};
// The XCLK line runs right past the Wi-Fi antenna and its harmonics can drown
// out the radio: at the usual 20 MHz this board managed 0.03 Mbit/s, at
// 24 MHz 2.3 Mbit/s and ~29 fps. Which clocks are clean depends on the Wi-Fi
// channel, so it's adjustable from the UI and saved across reboots. See
// https://github.com/espressif/arduino-esp32/issues/4655
static const int DEFAULT_XCLK = 24000000;
static int xclk = DEFAULT_XCLK;
static Preferences prefs;

// Rotation is the UI's "Rotate 180°" toggle, which the page sends on connect.
static int framesize = FRAMESIZE_VGA;
static int quality = 12;
static int rotate = 0;

static void apply_settings() {
  sensor_t *s = esp_camera_sensor_get();
  if (!s) return;
  s->set_framesize(s, (framesize_t)framesize);
  s->set_quality(s, quality);
  s->set_vflip(s, rotate);
  s->set_hmirror(s, rotate);
}

bool camera_start(int xclk_hz) {
  if (running) return true;
  prefs.begin("camera", false);
  if (xclk_hz >= 6000000 && xclk_hz <= 27000000) {
    xclk = xclk_hz;
    prefs.putInt("xclk", xclk);
  } else {
    xclk = prefs.getInt("xclk", DEFAULT_XCLK);
  }
  prefs.end();

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = xclk;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    // Allocate for the largest size up front so the resolution can be raised
    // later from the web UI. GRAB_LATEST always streams the newest frame
    // instead of a queued stale one, which keeps video latency down.
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    if (framesize > FRAMESIZE_VGA) framesize = FRAMESIZE_VGA;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  apply_settings();
  running = true;
  Serial.printf("Camera running, XCLK %d MHz, PSRAM %s\n", xclk / 1000000, psramFound() ? "found" : "not found");
  return true;
}

void camera_stop() {
  if (!running) return;
  running = false;
  // Let the stream hand back its current frame before the buffers go away.
  for (int i = 0; i < 300 && frames_out > 0; i++) delay(10);
  esp_camera_deinit();
  Serial.println("Camera stopped");
}

bool camera_running() { return running; }
int camera_xclk() { return xclk; }

camera_fb_t *camera_grab() {
  // Count the frame out before checking `running`, so camera_stop() either
  // sees this grab and waits for it, or this grab sees the camera stopping.
  frames_out++;
  if (!running) {
    frames_out--;
    return NULL;
  }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) frames_out--;
  return fb;
}

void camera_release(camera_fb_t *fb) {
  esp_camera_fb_return(fb);
  frames_out--;
}

void camera_set(char setting, int value) {
  switch (setting) {
    case 'F':
      if (value < FRAMESIZE_QVGA || value > FRAMESIZE_XGA) return;
      framesize = value;
      break;
    case 'Q':
      quality = constrain(value, 4, 63);
      break;
    case 'R':
      rotate = value ? 1 : 0;
      break;
    default:
      return;
  }
  if (running) apply_settings();
}
