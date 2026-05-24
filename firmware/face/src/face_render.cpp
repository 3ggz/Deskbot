// firmware/face/src/face_render.cpp
#include "face_render.h"
#include "Display_ST7701.h"

static uint16_t *g_fb = nullptr;   // 480 * 480 RGB565 framebuffer (PSRAM)

bool face_render_init() {
    if (g_fb) return true;
    g_fb = (uint16_t *)heap_caps_malloc(
        FACE_WIDTH * FACE_HEIGHT * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!g_fb) {
        Serial.println("LOG fb_alloc_failed");
        return false;
    }
    fb_fill(0x0000);  // start black
    return true;
}

void fb_fill(uint16_t color) {
    if (!g_fb) return;
    const int n = FACE_WIDTH * FACE_HEIGHT;
    for (int i = 0; i < n; ++i) g_fb[i] = color;
}

void fb_fill_round_rect(int x, int y, int w, int h, int radius, uint16_t color) {
    if (!g_fb) return;
    // Clamp to framebuffer bounds.
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > FACE_WIDTH)  w = FACE_WIDTH  - x;
    if (y + h > FACE_HEIGHT) h = FACE_HEIGHT - y;
    if (w <= 0 || h <= 0) return;
    if (radius < 0) radius = 0;
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;

    const int r2 = radius * radius;
    for (int row = 0; row < h; ++row) {
        // Vertical distance from the arc origin (matters only in the corner zones).
        int dy = 0;
        if      (row < radius)      dy = radius - row;
        else if (row >= h - radius) dy = row - (h - radius) + 1;
        for (int col = 0; col < w; ++col) {
            int dx = 0;
            if      (col < radius)      dx = radius - col;
            else if (col >= w - radius) dx = col - (w - radius) + 1;
            // Only check distance if we're actually in a corner cell.
            bool in_corner = (dy != 0 && dx != 0);
            if (!in_corner || dx * dx + dy * dy <= r2) {
                g_fb[(y + row) * FACE_WIDTH + (x + col)] = color;
            }
        }
    }
}

void fb_push_to_lcd() {
    if (!g_fb) return;
    // LCD_addWindow expects (x0, y0, x1, y1, uint8_t*) with x1/y1 INCLUSIVE.
    LCD_addWindow(0, 0, FACE_WIDTH - 1, FACE_HEIGHT - 1, (uint8_t *)g_fb);
}

void face_render_state(const EyePair &p, uint16_t color) {
    if (!g_fb) return;
    fb_fill(0x0000);

    auto draw = [&](const EyeShape &s) {
        int x = (FACE_WIDTH  / 2) + s.x_offset - s.width  / 2;
        int y = (FACE_HEIGHT / 2) + s.y_offset - s.height / 2;
        int r = max(max(s.radius_tl, s.radius_tr), max(s.radius_bl, s.radius_br));
        fb_fill_round_rect(x, y, s.width, s.height, r, color);
    };

    draw(p.left);
    draw(p.right);
    fb_push_to_lcd();
}

void face_render_emotion(EmotionId id, uint16_t color) {
    if (!g_fb) return;
    fb_fill(0x0000);  // black background
    const EyePair &p = EMOTIONS[id];

    auto draw = [&](const EyeShape &s) {
        int x = (FACE_WIDTH  / 2) + s.x_offset - s.width  / 2;
        int y = (FACE_HEIGHT / 2) + s.y_offset - s.height / 2;
        // Use the max corner radius as a uniform radius for now (Task 13+ may improve).
        int r = max(max(s.radius_tl, s.radius_tr), max(s.radius_bl, s.radius_br));
        fb_fill_round_rect(x, y, s.width, s.height, r, color);
    };

    draw(p.left);
    draw(p.right);
    fb_push_to_lcd();
}
