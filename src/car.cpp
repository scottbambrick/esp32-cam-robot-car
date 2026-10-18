#include <Arduino.h>
#include "car.h"

// Pin map from the DroneBot Workshop ESP32-CAM robot car (TB6612FNG driver).
#define MTR_PWM    12   // PWMA and PWMB, both tied to this one pin
#define LEFT_M0    15
#define LEFT_M1    14
#define RIGHT_M0   13
#define RIGHT_M1    2
#define LED_PIN     4   // onboard flash LED

// With both PWM inputs on one pin the two sides can't run at different speeds,
// so GPIO12 is held high and the direction inputs are PWM'd instead. The
// TB6612 then alternates drive/coast, which gives independent speed per side.
// LEDC channels 0/1 share timer 0 with the camera clock, so start at 2.
static const int CH_L0 = 2, CH_L1 = 3, CH_R0 = 4, CH_R1 = 5, CH_LED = 7;
static const int MOTOR_PWM_HZ = 20000;  // above hearing range, no whine

// The gear motors don't turn below about 160/255 duty (measured: the car
// first moved at 46% with an 80 minimum), so 1..100% is mapped onto
// MIN_DUTY..255 and any stick movement at all gets the car rolling.
static const int MIN_DUTY = 155;

// The original firmware's "right" command ran the LEFT_M pins backwards and
// the RIGHT_M pins forwards, i.e. LEFT_M drives the car's right-hand side.
static const bool SWAP_SIDES = true;

// Stop if the controller goes quiet (tab closed, Wi-Fi dropped, ...).
static const uint32_t FAILSAFE_MS = 400;

// After stopping, short-brake (both inputs high) this long, then coast so the
// driver isn't left holding the motors shorted.
static const uint32_t BRAKE_MS = 300;

// Commands set a target; car_loop() moves towards it. Speeding up is ramped
// so starts are smooth and the motors' inrush current doesn't upset the
// ESP32's supply. Slowing down is immediate, and reversing goes via zero.
struct Side {
  int ch_fwd, ch_rev;
  volatile int target;   // percent, -100..100
  float current;
  uint32_t brake_until;
};
static Side sides[2] = {{CH_L0, CH_L1, 0, 0, 0}, {CH_R0, CH_R1, 0, 0, 0}};

static volatile int accel_rate = 400;   // percent per second, 0 = instant
static volatile bool brake_enabled = true;
static volatile uint32_t last_command = 0;
static uint32_t last_loop_us = 0;

static void output(Side &s, int pct, bool braking) {
  if (braking) {
    ledcWrite(s.ch_fwd, 255);
    ledcWrite(s.ch_rev, 255);
    return;
  }
  uint32_t duty = pct == 0 ? 0 : MIN_DUTY + (255 - MIN_DUTY) * abs(pct) / 100;
  ledcWrite(s.ch_fwd, pct > 0 ? duty : 0);
  ledcWrite(s.ch_rev, pct < 0 ? duty : 0);
}

static void update(Side &s, float dt, uint32_t now) {
  int target = s.target;
  float cur = s.current;

  if (target == 0) {
    if (cur != 0) s.brake_until = brake_enabled ? now + BRAKE_MS : 0;
    cur = 0;
  } else {
    if (cur != 0 && (target > 0) != (cur > 0)) cur = 0;   // reversing
    if (abs(target) <= fabsf(cur) || accel_rate == 0) {
      cur = target;
    } else {
      float step = accel_rate * dt;
      cur = target > 0 ? min(cur + step, (float)target) : max(cur - step, (float)target);
    }
    s.brake_until = 0;
  }

  s.current = cur;
  int pct = (int)lroundf(cur);
  output(s, pct, pct == 0 && (int32_t)(s.brake_until - now) > 0);
}

void car_setup() {
  const int pins[] = {LEFT_M0, LEFT_M1, RIGHT_M0, RIGHT_M1};
  const int chans[] = {CH_L0, CH_L1, CH_R0, CH_R1};
  for (int i = 0; i < 4; i++) {
    ledcSetup(chans[i], MOTOR_PWM_HZ, 8);
    ledcAttachPin(pins[i], chans[i]);
    ledcWrite(chans[i], 0);
  }
  pinMode(MTR_PWM, OUTPUT);
  digitalWrite(MTR_PWM, HIGH);

  ledcSetup(CH_LED, 5000, 8);
  ledcAttachPin(LED_PIN, CH_LED);
  ledcWrite(CH_LED, 0);
}

void car_drive(int left, int right) {
  left = constrain(left, -100, 100);
  right = constrain(right, -100, 100);
  sides[0].target = SWAP_SIDES ? right : left;   // LEFT_M pins
  sides[1].target = SWAP_SIDES ? left : right;   // RIGHT_M pins
  last_command = millis();
}

void car_stop() {
  sides[0].target = 0;
  sides[1].target = 0;
}

void car_loop() {
  uint32_t now_us = micros();
  float dt = last_loop_us ? (now_us - last_loop_us) / 1e6f : 0;
  last_loop_us = now_us;
  uint32_t now = millis();

  bool moving = sides[0].target != 0 || sides[1].target != 0;
  if (moving && now - last_command > FAILSAFE_MS) {
    car_stop();
    Serial.println("Failsafe stop: no drive commands");
  }
  update(sides[0], dt, now);
  update(sides[1], dt, now);
}

void car_set_accel(int percent_per_second) {
  accel_rate = constrain(percent_per_second, 0, 5000);
}

void car_set_brake(bool enabled) {
  brake_enabled = enabled;
}

void car_light(uint8_t duty) {
  ledcWrite(CH_LED, duty);
}
