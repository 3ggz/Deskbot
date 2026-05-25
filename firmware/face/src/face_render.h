// firmware/face/src/face_render.h
// Software drawing primitives for Pip's face.
// All drawing goes into a 480x480 RGB565 framebuffer in PSRAM,
// then we push the whole buffer to the LCD via LCD_addWindow().
#pragma once
#include <Arduino.h>
#include "emotions.h"
#include "state.h"

constexpr int FACE_WIDTH  = 480;
constexpr int FACE_HEIGHT = 480;

// Initialize the framebuffer (allocates 480*480*2 = 460800 bytes in PSRAM).
// Returns true on success.
bool face_render_init();

// Draw a static face for the given emotion. Pushes the result to the LCD.
void face_render_emotion(EmotionId id, uint16_t color);

// Draw a face from an interpolated EyePair + color. Pushes the result to the LCD.
void face_render_state(const struct EyePair &pair, uint16_t color, int glance_x_offset);

// Low-level helpers — exposed for later tasks (interpolation, blink, etc).
void fb_fill(uint16_t color);
void fb_fill_round_rect(int x, int y, int w, int h, int radius, uint16_t color);
void fb_push_to_lcd();

// Caption now supports multiple lines. Pass two lines (one or both can be nullptr/empty).
void face_render_state_with_caption(const struct EyePair &pair, uint16_t color,
                                    int glance_x_offset,
                                    const char *line1, const char *line2);

// Low-level text helpers (exposed in case other tasks want them).
void fb_draw_char(int x, int y, char c, int scale, uint16_t color);
void fb_draw_string(int x, int y, const char *s, int scale, uint16_t color);
int  fb_string_width(const char *s, int scale);  // pixels for sizing the bubble
