#pragma once
#include <stdint.h>

void car_setup();
// Speeds are percentages, -100 (full reverse) to 100 (full forward). They set
// a target that car_loop() ramps towards.
void car_drive(int left, int right);
void car_stop();
// Call from loop() every few ms: applies ramping and braking, and stops the
// motors if commands stop arriving.
void car_loop();
// How fast speed may increase, in percent per second (0 = no ramp).
void car_set_accel(int percent_per_second);
// Short-brake when stopping instead of coasting.
void car_set_brake(bool enabled);
void car_light(uint8_t duty);
