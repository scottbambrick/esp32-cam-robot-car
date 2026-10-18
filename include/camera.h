#pragma once
#include "esp_camera.h"

// xclk_hz of 0 uses the saved clock (24 MHz by default); any other value in
// 6-27 MHz is used and saved.
bool camera_start(int xclk_hz);
void camera_stop();
bool camera_running();
int camera_xclk();

// Frame access for the HTTP handlers; NULL while the camera is stopped.
// Every frame from camera_grab() must go back through camera_release().
camera_fb_t *camera_grab();
void camera_release(camera_fb_t *fb);

// 'F' frame size, 'Q' JPEG quality, 'R' rotate 180. Remembered and reapplied
// whenever the camera restarts.
void camera_set(char setting, int value);
