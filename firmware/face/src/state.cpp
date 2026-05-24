// firmware/face/src/state.cpp
#include "state.h"
#include <Arduino.h>

static const uint32_t SHAPE_TRANSITION_MS = 300;
static const uint32_t COLOR_TRANSITION_MS = 400;

float ease_cubic(float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return t < 0.5f ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2.0f;
}

static int16_t lerp_i16(int16_t a, int16_t b, float t) {
    return (int16_t)(a + (b - a) * t);
}

static int8_t lerp_i8(int8_t a, int8_t b, float t) {
    return (int8_t)(a + (b - a) * t);
}

static EyeShape lerp_shape(const EyeShape &a, const EyeShape &b, float t) {
    EyeShape r;
    r.width      = lerp_i16(a.width,  b.width,  t);
    r.height     = lerp_i16(a.height, b.height, t);
    r.radius_tl  = lerp_i16(a.radius_tl, b.radius_tl, t);
    r.radius_tr  = lerp_i16(a.radius_tr, b.radius_tr, t);
    r.radius_br  = lerp_i16(a.radius_br, b.radius_br, t);
    r.radius_bl  = lerp_i16(a.radius_bl, b.radius_bl, t);
    r.x_offset   = lerp_i16(a.x_offset, b.x_offset, t);
    r.y_offset   = lerp_i16(a.y_offset, b.y_offset, t);
    r.tilt_left  = lerp_i8(a.tilt_left,  b.tilt_left,  t);
    r.tilt_right = lerp_i8(a.tilt_right, b.tilt_right, t);
    return r;
}

static uint16_t lerp_color565(uint16_t a, uint16_t b, float t) {
    uint8_t ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
    uint8_t br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
    uint8_t r = ar + (int)((br - ar) * t);
    uint8_t g = ag + (int)((bg - ag) * t);
    uint8_t bz = ab + (int)((bb - ab) * t);
    return (r << 11) | (g << 5) | bz;
}

void state_init(FaceState &s) {
    s.current = EMOTIONS[EMO_NEUTRAL];
    s.target  = EMOTIONS[EMO_NEUTRAL];
    s.start   = EMOTIONS[EMO_NEUTRAL];
    s.current_color = s.target_color = s.start_color = 0x4DDF;
    s.transition_start_ms = 0;
    s.color_start_ms = 0;
    s.last_cmd_ms = 0;
    s.mode = MODE_ACTIVE;
}

void state_set_target_emotion(FaceState &s, EmotionId id, uint16_t color, uint32_t now_ms) {
    s.start = s.current;
    s.target = EMOTIONS[id];
    s.transition_start_ms = now_ms;
    s.start_color = s.current_color;
    s.target_color = color;
    s.color_start_ms = now_ms;
    s.last_cmd_ms = now_ms;
    s.mode = MODE_ACTIVE;
}

void state_update_animation(FaceState &s, uint32_t now_ms) {
    uint32_t dt_shape = now_ms - s.transition_start_ms;
    float ts = (float)dt_shape / SHAPE_TRANSITION_MS;
    if (ts >= 1.0f) {
        s.current = s.target;
    } else {
        float e = ease_cubic(ts);
        s.current.left  = lerp_shape(s.start.left,  s.target.left,  e);
        s.current.right = lerp_shape(s.start.right, s.target.right, e);
    }

    uint32_t dt_color = now_ms - s.color_start_ms;
    float tc = (float)dt_color / COLOR_TRANSITION_MS;
    if (tc >= 1.0f) {
        s.current_color = s.target_color;
    } else {
        s.current_color = lerp_color565(s.start_color, s.target_color, ease_cubic(tc));
    }
}
