# ESP32-CAM Robot Car

Firmware for an ESP32-CAM robot car with a full-screen, touch-friendly driving interface served by the car itself. The hardware and pin map are from the [DroneBot Workshop ESP32-CAM robot car](https://dronebotworkshop.com/esp32cam-robot-car/) (TB6612FNG motor driver). The firmware is a from-scratch rewrite.

## Features

- **Full-screen live video** with two overlaid joysticks: left for throttle, right for steering. Touch anywhere on either half and the stick follows your thumb.
- **Proportional steering.** Each side's speed is controlled independently, so the car drives smooth arcs rather than stop-and-spin.
- **Low-latency control** over a WebSocket at up to 25 updates a second. The car stops by itself if commands stop arriving for 400 ms.
- **Smooth driving:** acceleration ramping, active braking, car-style reversing and an adjustable response curve.
- **Adaptive video:** live fps readout, automatic resolution drop when the frame rate sags, and reconnection within a couple of seconds if frames stop.
- **One driver at a time.** A second phone can watch but can't fight the first for control.
- **Keyboard** (W A S D / arrows, L for the headlight) and **gamepad** support.
- **Add to Home Screen** for a full-screen, app-like launcher.
- **Wireless (OTA) firmware updates.** No IO0 jumper needed after the first flash.

## Setup

1. Copy `include/secrets.example.h` to `include/secrets.h` and fill in your Wi-Fi details and passwords.
2. Copy `secrets.example.ini` to `secrets.ini` and set `ota_password` to the same value as `OTA_PASSWORD`.
3. **First flash over USB:** jumper IO0 to GND, press reset, then run `pio run -e esp32cam -t upload`. Remove the jumper and reset.
4. **After that, flash over Wi-Fi:** `pio run -e ota -t upload`. If `robot.local` doesn't resolve, add `--upload-port <car's IP>`.

Open `http://robot.local`, or the IP printed on the serial monitor. If the car can't reach your Wi-Fi, it starts its own access point (`ESP32-CAM Robot`) at `http://192.168.4.1`.

## Wiring

| ESP32-CAM | TB6612FNG |
|---|---|
| GPIO 12 | PWMA and PWMB |
| GPIO 15 / 14 | Motor A IN1 / IN2 |
| GPIO 13 / 2 | Motor B IN1 / IN2 |
| 3.3 V | STBY |

GPIO 12 is held high and the IN pins are PWM'd instead, which gives independent speed per side even though both PWM inputs share one pin. If the steering or throttle runs the wrong way, use the toggles in the page's settings. You can also flip `SWAP_SIDES` in `src/car.cpp`.

## Video troubleshooting

The ESP32-CAM's camera clock (XCLK) runs next to the Wi-Fi antenna, and its harmonics can swamp the radio ([arduino-esp32#4655](https://github.com/espressif/arduino-esp32/issues/4655)). On this board, the usual 20 MHz gave 0.4 fps, and 24 MHz gives 20–29 fps. The clock is selectable under **Settings → Video → Camera clock** and is saved on the car. Which speeds are clean depends on the Wi-Fi channel, so try another if video stutters. Foil tape over the camera ribbon or an external antenna also helps.

`http://robot.local/set` reports the camera clock, Wi-Fi channel, signal strength and free memory.

## Layout

| File | Purpose |
|---|---|
| `src/main.cpp` | Wi-Fi, mDNS and OTA setup |
| `src/camera.cpp` | Camera init, frame access, saved clock setting |
| `src/car.cpp` | Motor driver: ramping, braking, failsafe |
| `src/web_server.cpp` | HTTP and WebSocket on port 80, MJPEG stream on port 81 |
| `include/index_html.h` | The control page (self-contained, works without internet) |
| `include/icons.h` | Home-screen icons |
