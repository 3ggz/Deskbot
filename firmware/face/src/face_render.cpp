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

static void fb_fill_ellipse(int cx, int cy, int rx, int ry, uint16_t color) {
    if (!g_fb) return;
    if (rx <= 0 || ry <= 0) return;
    int x0 = max(0, cx - rx);
    int y0 = max(0, cy - ry);
    int x1 = min(FACE_WIDTH  - 1, cx + rx);
    int y1 = min(FACE_HEIGHT - 1, cy + ry);
    float inv_rx2 = 1.0f / (float)(rx * rx);
    float inv_ry2 = 1.0f / (float)(ry * ry);
    for (int py = y0; py <= y1; ++py) {
        int dy = py - cy;
        float ny2 = (float)(dy * dy) * inv_ry2;
        if (ny2 > 1.0f) continue;
        for (int px = x0; px <= x1; ++px) {
            int dx = px - cx;
            float nx2 = (float)(dx * dx) * inv_rx2;
            if (nx2 + ny2 <= 1.0f) {
                g_fb[py * FACE_WIDTH + px] = color;
            }
        }
    }
}

// Software rotation: draw a rounded rect centered at (cx, cy) tilted by angle_deg.
// Positive angle = counter-clockwise in screen-space (note: y-axis points DOWN on screen).
// Uses inverse-mapping: for each pixel in the bounding box, rotate it back into the
// un-rotated rect's space and apply the same rounded-rect test as fb_fill_round_rect.
static void fb_fill_round_rect_tilted(int cx, int cy, int w, int h, int radius, float angle_deg, uint16_t color) {
    if (!g_fb) return;
    if (radius < 0) radius = 0;
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    const int r2 = radius * radius;

    // Bounding box big enough to contain the rotated rect (diagonal radius + 1 px slack).
    int half_diag = (int)ceilf(sqrtf((float)(w*w + h*h)) * 0.5f) + 1;
    int x0 = max(0, cx - half_diag);
    int y0 = max(0, cy - half_diag);
    int x1 = min(FACE_WIDTH  - 1, cx + half_diag);
    int y1 = min(FACE_HEIGHT - 1, cy + half_diag);

    float rad = angle_deg * 3.14159265f / 180.0f;
    float cs = cosf(rad);
    float sn = sinf(rad);

    // Half-extents for local-coord origin shift.
    float half_w = w * 0.5f;
    float half_h = h * 0.5f;

    for (int py = y0; py <= y1; ++py) {
        for (int px = x0; px <= x1; ++px) {
            // Translate to eye center, then inverse-rotate to un-tilted frame.
            float dx = px - cx;
            float dy = py - cy;
            // Inverse rotation by -angle: (rx, ry) in un-tilted local frame, origin at eye center.
            float rx =  dx * cs + dy * sn;
            float ry = -dx * sn + dy * cs;
            // Shift origin to top-left of un-tilted rect.
            int ilx = (int)floorf(rx + half_w);
            int ily = (int)floorf(ry + half_h);
            if (ilx < 0 || ilx >= w || ily < 0 || ily >= h) continue;

            // Same rounded-rect test as fb_fill_round_rect.
            int kdx = 0, kdy = 0;
            if      (ily < radius)        kdy = radius - ily;
            else if (ily >= h - radius)   kdy = ily - (h - radius) + 1;
            if      (ilx < radius)        kdx = radius - ilx;
            else if (ilx >= w - radius)   kdx = ilx - (w - radius) + 1;
            bool in_corner = (kdy != 0 && kdx != 0);
            if (!in_corner || kdx * kdx + kdy * kdy <= r2) {
                g_fb[py * FACE_WIDTH + px] = color;
            }
        }
    }
}

void fb_push_to_lcd() {
    if (!g_fb) return;
    // LCD_addWindow expects (x0, y0, x1, y1, uint8_t*) with x1/y1 INCLUSIVE.
    LCD_addWindow(0, 0, FACE_WIDTH - 1, FACE_HEIGHT - 1, (uint8_t *)g_fb);
}

void face_render_state(const EyePair &p, uint16_t color, int glance_x_offset) {
    if (!g_fb) return;
    fb_fill(0x0000);

    // Build a lighter pupil color: blend current eye color with white at ~60%.
    auto lighten = [&](uint16_t c) -> uint16_t {
        uint8_t r = (c >> 11) & 0x1F;
        uint8_t g = (c >> 5)  & 0x3F;
        uint8_t b =  c        & 0x1F;
        // 60% toward white per channel.
        r = (uint8_t)(r + (0x1F - r) * 0.6f);
        g = (uint8_t)(g + (0x3F - g) * 0.6f);
        b = (uint8_t)(b + (0x1F - b) * 0.6f);
        return (uint16_t)((r << 11) | (g << 5) | b);
    };
    uint16_t pupil_color = lighten(color);

    auto draw_eye = [&](const EyeShape &s, int8_t tilt, int gaze_offset) {
        int cx = (FACE_WIDTH  / 2) + s.x_offset;
        int cy = (FACE_HEIGHT / 2) + s.y_offset;
        int r  = max(max(s.radius_tl, s.radius_tr), max(s.radius_bl, s.radius_br));

        // 1. Eye shape (tilted if needed).
        if (tilt != 0) {
            fb_fill_round_rect_tilted(cx, cy, s.width, s.height, r, (float)tilt, color);
        } else {
            fb_fill_round_rect(cx - s.width / 2, cy - s.height / 2, s.width, s.height, r, color);
        }

        // 2. Pupil dimensions scale with the CURRENT eye dimensions, so blinking
        //    (height collapses) and breathing (subtle ±5% oscillation) naturally
        //    affect the pupil too. Vertical ellipse: roughly 1/3 the eye width
        //    by 1/2 the eye height as a baseline.
        int pupil_rx = s.width  / 6;
        int pupil_ry = s.height / 6;

        // Keep the pupil safely inside the eye even when it's small.
        int max_pupil_rx = (s.width  / 2) - 4;
        int max_pupil_ry = (s.height / 2) - 4;
        if (pupil_rx > max_pupil_rx) pupil_rx = max_pupil_rx;
        if (pupil_ry > max_pupil_ry) pupil_ry = max_pupil_ry;

        // If the eye is too thin or too short for a visible pupil, skip it
        // (clean blink — eye becomes a slit with no floating pupil).
        if (pupil_rx < 3 || pupil_ry < 3) return;

        // 3. Clamp pupil position so it stays inside the eye even at peak gaze.
        int max_shift_x = (s.width  / 2) - pupil_rx - 4;
        if (max_shift_x < 0) max_shift_x = 0;
        int max_shift_y = (s.height / 2) - pupil_ry - 4;
        if (max_shift_y < 0) max_shift_y = 0;

        // 5. Position: center + clamped gaze offset. Slight upward bias for "alert" look.
        int desired_shift = gaze_offset;
        if (desired_shift >  max_shift_x) desired_shift =  max_shift_x;
        if (desired_shift < -max_shift_x) desired_shift = -max_shift_x;
        int px = cx + desired_shift;
        int py = cy - (max_shift_y / 4);

        fb_fill_ellipse(px, py, pupil_rx, pupil_ry, pupil_color);
    };

    draw_eye(p.left,  p.left.tilt_left,   glance_x_offset);
    draw_eye(p.right, p.right.tilt_right, glance_x_offset);

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
