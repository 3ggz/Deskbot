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

    // Auto-return-to-baseline tracking
    bool returned_to_baseline;

    // Caption overlay — multi-line, paginated.
    static const int MAX_LINES = 8;
    static const int MAX_CHARS_PER_LINE = 32;  // ~30 fits, leave a couple slack
    char     caption_lines[MAX_LINES][MAX_CHARS_PER_LINE];
    int      caption_line_count;
    int      caption_current_page;   // which 2-line window we're showing
    uint32_t caption_until_ms;
    uint32_t caption_page_advance_ms;
    bool     caption_live_mode;      // true = show last 2 lines (typing); false = paginate
};

void state_init(FaceState &s);
void state_set_target_emotion(FaceState &s, EmotionId id, uint16_t color, uint32_t now_ms);
void state_update_animation(FaceState &s, uint32_t now_ms);

void state_trigger_blink(FaceState &s, uint32_t now_ms);
void state_schedule_next_blink(FaceState &s, uint32_t now_ms);
void state_trigger_glance(FaceState &s, uint32_t now_ms);

void state_set_caption(FaceState &s, const char *text, uint32_t duration_ms, uint32_t now_ms);
void state_set_caption_live(FaceState &s, const char *text, uint32_t now_ms);  // typing mode
void state_clear_caption(FaceState &s);
void state_tick_caption(FaceState &s, uint32_t now_ms);  // advances page if needed

float ease_cubic(float t);

// Returns the current pupil X offset including smooth glance animation + always-on subtle drift.
int state_current_pupil_offset(const FaceState &s, uint32_t now_ms);
