// firmware/face/src/touch.h
// Minimal CST816 capacitive touch driver. Returns whether currently touched.
#pragma once
#include <Arduino.h>

void touch_init();
bool touch_poll(int *out_x, int *out_y);  // true if touched right now
