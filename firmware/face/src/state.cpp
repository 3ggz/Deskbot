// firmware/face/src/state.cpp
#include "state.h"
#include <Arduino.h>

static const uint32_t SHAPE_TRANSITION_MS = 300;
static const uint32_t COLOR_TRANSITION_MS = 400;
static const uint32_t BLINK_DURATION_MS   = 200;   // close 80 + hold 40 + open 80
static const int16_t  BLINK_MIN_HEIGHT    = 10;
static const uint32_t GLANCE_DURATION_MS  = 400;
static const int8_t   GLANCE_MAX_OFFSET   = 20;
static const float    BREATHE_AMP         = 0.05f;
static const float    BREATHE_PERIOD_MS   = 3000.0f;

// Idle progression thresholds (milliseconds since last_cmd_ms)
static const uint32_t IDLE_BLINK_AT    = 5000;
static const uint32_t IDLE_GLANCE_AT   = 35000;
static const uint32_t IDLE_BREATHE_AT  = 95000;
static const uint32_t IDLE_SLEEPY_AT   = 215000;

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

void state_schedule_next_blink(FaceState &s, uint32_t now_ms) {
    s.next_blink_ms = now_ms + 3000 + (uint32_t)random(2000);  // 3-5s
}

void state_trigger_blink(FaceState &s, uint32_t now_ms) {
    s.blinking = true;
    s.blink_start_ms = now_ms;
    s.next_blink_ms = now_ms + BLINK_DURATION_MS + 3000 + (uint32_t)random(2000);
}

void state_trigger_glance(FaceState &s, uint32_t now_ms) {
    s.glance_x_offset = (random(2) == 0) ? -GLANCE_MAX_OFFSET : GLANCE_MAX_OFFSET;
    s.glance_end_ms = now_ms + GLANCE_DURATION_MS;
    s.next_glance_ms = now_ms + 15000 + (uint32_t)random(10000);  // 15-25s
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

    s.blinking = false;
    s.blink_start_ms = 0;
    state_schedule_next_blink(s, 0);

    s.next_glance_ms = 0;
    s.glance_x_offset = 0;
    s.glance_end_ms = 0;
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
    // 1. Shape interpolation toward target.
    uint32_t dt_shape = now_ms - s.transition_start_ms;
    float ts = (float)dt_shape / SHAPE_TRANSITION_MS;
    if (ts >= 1.0f) {
        s.current = s.target;
    } else {
        float e = ease_cubic(ts);
        s.current.left  = lerp_shape(s.start.left,  s.target.left,  e);
        s.current.right = lerp_shape(s.start.right, s.target.right, e);
    }

    // 2. Color crossfade toward target.
    uint32_t dt_color = now_ms - s.color_start_ms;
    float tc = (float)dt_color / COLOR_TRANSITION_MS;
    if (tc >= 1.0f) {
        s.current_color = s.target_color;
    } else {
        s.current_color = lerp_color565(s.start_color, s.target_color, ease_cubic(tc));
    }

    // 3. Idle progression based on time since last command.
    uint32_t idle_dt = now_ms - s.last_cmd_ms;
    FaceMode prev_mode = s.mode;
    if      (idle_dt < IDLE_BLINK_AT)    s.mode = MODE_ACTIVE;
    else if (idle_dt < IDLE_GLANCE_AT)   s.mode = MODE_IDLE_BLINK;
    else if (idle_dt < IDLE_BREATHE_AT)  s.mode = MODE_IDLE_GLANCE;
    else if (idle_dt < IDLE_SLEEPY_AT)   s.mode = MODE_IDLE_BREATHE;
    else                                 s.mode = MODE_SLEEPY;

    // Just entered SLEEPY: animate decay to the sleepy shape.
    if (s.mode == MODE_SLEEPY && prev_mode != MODE_SLEEPY) {
        s.start = s.current;
        s.target = EMOTIONS[EMO_SLEEPY];
        s.transition_start_ms = now_ms;
    }

    // 4. Glance — only active in GLANCE/BREATHE (NOT sleepy).
    if (s.mode == MODE_IDLE_GLANCE || s.mode == MODE_IDLE_BREATHE) {
        if (now_ms >= s.next_glance_ms) {
            state_trigger_glance(s, now_ms);
        }
        if (now_ms < s.glance_end_ms) {
            s.current.left.x_offset  += s.glance_x_offset;
            s.current.right.x_offset += s.glance_x_offset;
        }
    }

    // 5. Breathe — in BREATHE and SLEEPY (slow gentle pulse).
    if (s.mode == MODE_IDLE_BREATHE || s.mode == MODE_SLEEPY) {
        float phase = ((now_ms % (uint32_t)BREATHE_PERIOD_MS) / BREATHE_PERIOD_MS) * 2.0f * (float)PI;
        float scale = 1.0f + sinf(phase) * BREATHE_AMP;
        s.current.left.width   = (int16_t)(s.current.left.width   * scale);
        s.current.left.height  = (int16_t)(s.current.left.height  * scale);
        s.current.right.width  = (int16_t)(s.current.right.width  * scale);
        s.current.right.height = (int16_t)(s.current.right.height * scale);
    }

    // 6. Blink — override height temporarily. Auto-scheduled in IDLE_BLINK and beyond.
    if (s.blinking) {
        uint32_t bt = now_ms - s.blink_start_ms;
        if (bt >= BLINK_DURATION_MS) {
            s.blinking = false;
        } else {
            float bp;
            if      (bt < 80)  bp = (float)bt / 80.0f;                     // closing
            else if (bt < 120) bp = 1.0f;                                  // held
            else               bp = 1.0f - (float)(bt - 120) / 80.0f;      // opening
            int16_t lh = s.current.left.height;
            int16_t rh = s.current.right.height;
            s.current.left.height  = lerp_i16(lh, BLINK_MIN_HEIGHT, bp);
            s.current.right.height = lerp_i16(rh, BLINK_MIN_HEIGHT, bp);
        }
    } else if (s.mode != MODE_ACTIVE && now_ms >= s.next_blink_ms) {
        // Auto-blink only in idle states (not while a user-driven emotion just changed).
        state_trigger_blink(s, now_ms);
    }
}
