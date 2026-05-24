// firmware/face/src/state.h
// FaceState: current shape, target shape, color, and animation timing
// + idle progression (blink, glance, breathe, sleepy).
#pragma once
#include "emotions.h"

enum FaceMode : uint8_t {
    MODE_ACTIVE = 0,
    MODE_IDLE_BLINK,
    MODE_IDLE_GLANCE,
    MODE_IDLE_BREATHE,
    MODE_SLEEPY,
};

struct FaceState {
    EyePair  current;            // interpolated, drawn this frame
    EyePair  target;             // animating toward this
    EyePair  start;              // shape at transition_start_ms
    uint16_t current_color;
    uint16_t target_color;
    uint16_t start_color;
    uint32_t transition_start_ms;
    uint32_t color_start_ms;
    uint32_t last_cmd_ms;
    FaceMode mode;

    // Blink scheduling
    bool     blinking;
    uint32_t blink_start_ms;
    uint32_t next_blink_ms;

    // Glance
    uint32_t next_glance_ms;
    int8_t   glance_x_offset;
    uint32_t glance_end_ms;
};

void state_init(FaceState &s);
void state_set_target_emotion(FaceState &s, EmotionId id, uint16_t color, uint32_t now_ms);
void state_update_animation(FaceState &s, uint32_t now_ms);

void state_trigger_blink(FaceState &s, uint32_t now_ms);
void state_schedule_next_blink(FaceState &s, uint32_t now_ms);
void state_trigger_glance(FaceState &s, uint32_t now_ms);

float ease_cubic(float t);
