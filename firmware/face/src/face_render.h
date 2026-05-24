// firmware/face/src/face_render.h
// Software drawing primitives for Pip's face.
// All drawing goes into a 480x480 RGB565 framebuffer in PSRAM,
// then we push the whole buffer to the LCD via LCD_addWindow().
#pragma once
#include <Arduino.h>
#include "emotions.h"

constexpr int FACE_WIDTH  = 480;
constexpr int FACE_HEIGHT = 480;

// Initialize the framebuffer (allocates 480*480*2 = 460800 bytes in PSRAM).
// Returns true on success.
bool face_render_init();

// Draw a static face for the given emotion. Pushes the result to the LCD.
void face_render_emotion(EmotionId id, uint16_t color);

// Low-level helpers — exposed for later tasks (interpolation, blink, etc).
void fb_fill(uint16_t color);
void fb_fill_round_rect(int x, int y, int w, int h, int radius, uint16_t color);
void fb_push_to_lcd();
