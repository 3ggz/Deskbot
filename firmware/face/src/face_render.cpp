// firmware/face/src/face_render.cpp
#include "face_render.h"
#include "Display_ST7701.h"

static uint16_t *g_fb = nullptr;   // 480 * 480 RGB565 framebuffer (PSRAM)

// Minimal 5x7 bitmap font for ASCII 0x20..0x7E. Public domain glyphs.
// Each glyph is 5 bytes — one byte per column, LSB = top pixel of column.
// This is the classic "font5x7" / "MikroElektronika" table.
static const uint8_t FONT5X7[95][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 0x20 ' '
    {0x00,0x00,0x5F,0x00,0x00}, // '!'
    {0x00,0x07,0x00,0x07,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
    {0x23,0x13,0x08,0x64,0x62}, // '%'
    {0x36,0x49,0x55,0x22,0x50}, // '&'
    {0x00,0x05,0x03,0x00,0x00}, // '\''
    {0x00,0x1C,0x22,0x41,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00}, // ')'
    {0x08,0x2A,0x1C,0x2A,0x08}, // '*'
    {0x08,0x08,0x3E,0x08,0x08}, // '+'
    {0x00,0x50,0x30,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08}, // '-'
    {0x00,0x60,0x60,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46}, // '2'
    {0x21,0x41,0x45,0x4B,0x31}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x06,0x49,0x49,0x29,0x1E}, // '9'
    {0x00,0x36,0x36,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00}, // ';'
    {0x00,0x08,0x14,0x22,0x41}, // '<'
    {0x14,0x14,0x14,0x14,0x14}, // '='
    {0x41,0x22,0x14,0x08,0x00}, // '>'
    {0x02,0x01,0x51,0x09,0x06}, // '?'
    {0x32,0x49,0x79,0x41,0x3E}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x01,0x01}, // 'F'
    {0x3E,0x41,0x41,0x51,0x32}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x04,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x7F,0x20,0x18,0x20,0x7F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x03,0x04,0x78,0x04,0x03}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 'Z'
    {0x00,0x00,0x7F,0x41,0x41}, // '['
    {0x02,0x04,0x08,0x10,0x20}, // '\\'
    {0x41,0x41,0x7F,0x00,0x00}, // ']'
    {0x04,0x02,0x01,0x02,0x04}, // '^'
    {0x40,0x40,0x40,0x40,0x40}, // '_'
    {0x00,0x01,0x02,0x04,0x00}, // '`'
    {0x20,0x54,0x54,0x54,0x78}, // 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 'f'
    {0x08,0x14,0x54,0x54,0x3C}, // 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 'j'
    {0x00,0x7F,0x10,0x28,0x44}, // 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 'z'
    {0x00,0x08,0x36,0x41,0x00}, // '{'
    {0x00,0x00,0x7F,0x00,0x00}, // '|'
    {0x00,0x41,0x36,0x08,0x00}, // '}'
    {0x08,0x08,0x2A,0x1C,0x08}, // '~'
};

static const int CHAR_WIDTH   = 5;
static const int CHAR_HEIGHT  = 7;
static const int CHAR_SPACING = 1;

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

void fb_draw_char(int x, int y, char c, int scale, uint16_t color) {
    if (!g_fb) return;
    if (c < 0x20 || c > 0x7E) return;
    const uint8_t *glyph = FONT5X7[c - 0x20];
    for (int col = 0; col < CHAR_WIDTH; ++col) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < CHAR_HEIGHT; ++row) {
            if (bits & (1 << row)) {
                // Draw a scale×scale block for this pixel.
                int px = x + col * scale;
                int py = y + row * scale;
                for (int dy = 0; dy < scale; ++dy) {
                    int yy = py + dy;
                    if (yy < 0 || yy >= FACE_HEIGHT) continue;
                    for (int dx = 0; dx < scale; ++dx) {
                        int xx = px + dx;
                        if (xx < 0 || xx >= FACE_WIDTH) continue;
                        g_fb[yy * FACE_WIDTH + xx] = color;
                    }
                }
            }
        }
    }
}

void fb_draw_string(int x, int y, const char *s, int scale, uint16_t color) {
    if (!s) return;
    int cursor = x;
    for (const char *p = s; *p; ++p) {
        fb_draw_char(cursor, y, *p, scale, color);
        cursor += (CHAR_WIDTH + CHAR_SPACING) * scale;
    }
}

int fb_string_width(const char *s, int scale) {
    if (!s) return 0;
    int len = 0;
    for (const char *p = s; *p; ++p) ++len;
    if (len == 0) return 0;
    return (len * CHAR_WIDTH + (len - 1) * CHAR_SPACING) * scale;
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

        // Eyes that have collapsed to a slit (sleepy, mid-blink, winked-shut)
        // shouldn't show a pupil — Pip would look weirdly awake.
        if (s.height < 30) return;

        int pupil_rx = s.width  / 6;
        int pupil_ry = s.height / 6;

        // Keep the pupil safely inside the eye even when it's small.
        int max_pupil_rx = (s.width  / 2) - 4;
        int max_pupil_ry = (s.height / 2) - 4;
        if (pupil_rx > max_pupil_rx) pupil_rx = max_pupil_rx;
        if (pupil_ry > max_pupil_ry) pupil_ry = max_pupil_ry;

        // If the eye is too thin for a visible pupil, skip it.
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

        // 6. Eyebrow — small rounded rect above the eye, optionally tilted.
        if (s.brow_width >= 6 && s.brow_height >= 3) {
            int bx = cx;
            int by = cy + s.brow_y_offset;
            int br = s.brow_height / 2;  // fully-rounded ends (pill shape)
            if (s.brow_tilt != 0) {
                fb_fill_round_rect_tilted(bx, by, s.brow_width, s.brow_height, br, (float)s.brow_tilt, color);
            } else {
                fb_fill_round_rect(bx - s.brow_width / 2, by - s.brow_height / 2,
                                   s.brow_width, s.brow_height, br, color);
            }
        }
    };

    draw_eye(p.left,  p.left.tilt_left,   glance_x_offset);
    draw_eye(p.right, p.right.tilt_right, glance_x_offset);

    // Caption overlay is rendered by the wrapper that has access to state.
    fb_push_to_lcd();
}

void face_render_state_with_caption(const EyePair &p, uint16_t color,
                                    int glance_x_offset, const char *caption) {
    if (!g_fb) return;
    fb_fill(0x0000);

    auto lighten = [&](uint16_t c) -> uint16_t {
        uint8_t r = (c >> 11) & 0x1F;
        uint8_t g = (c >> 5)  & 0x3F;
        uint8_t b =  c        & 0x1F;
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

        if (tilt != 0) {
            fb_fill_round_rect_tilted(cx, cy, s.width, s.height, r, (float)tilt, color);
        } else {
            fb_fill_round_rect(cx - s.width / 2, cy - s.height / 2, s.width, s.height, r, color);
        }

        // Eyes that have collapsed to a slit (sleepy, mid-blink, winked-shut)
        // shouldn't show a pupil — Pip would look weirdly awake.
        if (s.height < 30) return;

        int pupil_rx = s.width  / 6;
        int pupil_ry = s.height / 6;
        int max_pupil_rx = (s.width  / 2) - 4;
        int max_pupil_ry = (s.height / 2) - 4;
        if (pupil_rx > max_pupil_rx) pupil_rx = max_pupil_rx;
        if (pupil_ry > max_pupil_ry) pupil_ry = max_pupil_ry;
        // If the eye is too thin for a visible pupil, skip it.
        if (!(pupil_rx < 3 || pupil_ry < 3)) {
            int max_shift_x = (s.width  / 2) - pupil_rx - 4;
            if (max_shift_x < 0) max_shift_x = 0;
            int max_shift_y = (s.height / 2) - pupil_ry - 4;
            if (max_shift_y < 0) max_shift_y = 0;
            int desired_shift = gaze_offset;
            if (desired_shift >  max_shift_x) desired_shift =  max_shift_x;
            if (desired_shift < -max_shift_x) desired_shift = -max_shift_x;
            int px = cx + desired_shift;
            int py = cy - (max_shift_y / 4);
            fb_fill_ellipse(px, py, pupil_rx, pupil_ry, pupil_color);
        }

        // Brow
        if (s.brow_width >= 6 && s.brow_height >= 3) {
            int bx = cx;
            int by = cy + s.brow_y_offset;
            int br = s.brow_height / 2;
            if (s.brow_tilt != 0) {
                fb_fill_round_rect_tilted(bx, by, s.brow_width, s.brow_height, br, (float)s.brow_tilt, color);
            } else {
                fb_fill_round_rect(bx - s.brow_width / 2, by - s.brow_height / 2,
                                   s.brow_width, s.brow_height, br, color);
            }
        }
    };

    draw_eye(p.left,  p.left.tilt_left,   glance_x_offset);
    draw_eye(p.right, p.right.tilt_right, glance_x_offset);

    // Caption overlay (drawn AFTER the face so it sits on top).
    if (caption && caption[0]) {
        const int scale   = 2;          // 5x7 font → 10x14 per char
        const int char_w  = (CHAR_WIDTH + CHAR_SPACING) * scale;
        const int char_h  = CHAR_HEIGHT * scale;
        const int max_chars = 36;       // truncate long captions to single line
        char truncated[max_chars + 1];
        int len = 0;
        for (const char *p2 = caption; *p2 && len < max_chars; ++p2, ++len) {
            truncated[len] = *p2;
        }
        truncated[len] = 0;

        int text_w = fb_string_width(truncated, scale);
        int box_w  = text_w + 32;       // horizontal padding
        if (box_w > 400) box_w = 400;
        int box_h  = char_h + 18;
        int box_x  = (FACE_WIDTH - box_w) / 2;
        int box_y  = FACE_HEIGHT - box_h - 32;  // 32px above bottom edge

        // Dark rounded box (dark blueish-grey).
        fb_fill_round_rect(box_x, box_y, box_w, box_h, 12, 0x18C3);

        // Thin colored border using the eye color — top and bottom strips.
        const int border = 2;
        for (int by2 = 0; by2 < border; ++by2) {
            for (int bx2 = 0; bx2 < box_w; ++bx2) {
                int yy = box_y + by2;
                int xx = box_x + bx2;
                if (yy >= 0 && yy < FACE_HEIGHT && xx >= 0 && xx < FACE_WIDTH)
                    g_fb[yy * FACE_WIDTH + xx] = color;
            }
        }
        for (int by2 = box_h - border; by2 < box_h; ++by2) {
            for (int bx2 = 0; bx2 < box_w; ++bx2) {
                int yy = box_y + by2;
                int xx = box_x + bx2;
                if (yy >= 0 && yy < FACE_HEIGHT && xx >= 0 && xx < FACE_WIDTH)
                    g_fb[yy * FACE_WIDTH + xx] = color;
            }
        }

        // White text centered in the box.
        int text_x = box_x + (box_w - text_w) / 2;
        int text_y = box_y + (box_h - char_h) / 2;
        fb_draw_string(text_x, text_y, truncated, scale, 0xFFFF);
    }

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
