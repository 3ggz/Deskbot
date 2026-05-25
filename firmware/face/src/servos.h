// firmware/face/src/servos.h
// Two-servo head control: pan (left/right) + tilt (up/down).
// Drives SG90s on GPIO43 (pan) and GPIO44 (tilt) via the ESP32 LEDC peripheral.
#pragma once
#include <Arduino.h>

void servos_init();

// Set absolute angles. 0..180 degrees, with 90 = center.
void servo_pan_to(int angle_deg);
void servo_tilt_to(int angle_deg);

// Named movement helpers. Trigger a non-blocking animation; servos_tick(now_ms)
// must be called regularly to advance them.
void servo_nod();           // tilt: center → down → up → center (yes)
void servo_shake();         // pan: center → left → right → center (no)
void servo_tilt_left();     // pan: small lean left, hold, return
void servo_tilt_right();    // pan: small lean right, hold, return
void servo_wiggle();        // alternating pan, several oscillations (dance)
void servo_idle();          // return to center

void servos_tick(uint32_t now_ms);  // call every render frame

int servo_pan_current();
int servo_tilt_current();

// Live tuning API
void servo_set_limits(int pan_min, int pan_max, int tilt_min, int tilt_max);
void servo_set_trim(int pan_trim, int tilt_trim);
int  servo_get_pan_min();
int  servo_get_pan_max();
int  servo_get_tilt_min();
int  servo_get_tilt_max();
int  servo_get_pan_trim();
int  servo_get_tilt_trim();
