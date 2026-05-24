// firmware/face/src/emotions.h
// 7 emotion shapes for Pip's face. Values from the design spec
// (docs/superpowers/specs/2026-05-23-stage-2-face-design.md).
#pragma once
#include <Arduino.h>

struct EyeShape {
    int16_t width;
    int16_t height;
    int16_t radius_tl, radius_tr, radius_br, radius_bl;
    int16_t x_offset;   // from center; eye pair is symmetric
    int16_t y_offset;
    int8_t  tilt_left;  // degrees
    int8_t  tilt_right;
    // Eyebrow
    int16_t brow_width;
    int16_t brow_height;
    int16_t brow_y_offset;  // from eye center; NEGATIVE = above eye
    int8_t  brow_tilt;      // degrees; positive = CCW (right end up in y-down screen)
};

// Index this table by emotion id; see EMOTION_NAMES below for the mapping.
enum EmotionId : uint8_t {
    EMO_NEUTRAL = 0,
    EMO_HAPPY,
    EMO_EXCITED,
    EMO_SURPRISED,
    EMO_THINKING,
    EMO_SAD,
    EMO_SLEEPY,
    EMOTION_COUNT
};

static const char* EMOTION_NAMES[EMOTION_COUNT] = {
    "neutral", "happy", "excited", "surprised", "thinking", "sad", "sleepy"
};

// Symmetric pair: index [emo].left and [emo].right
struct EyePair { EyeShape left; EyeShape right; };

static const EyePair EMOTIONS[EMOTION_COUNT] = {
    // neutral — default rounded bars, flat brows above
    { {108, 162, 42, 42, 42, 42, -110, 0, 0, 0,   80, 14, -110,   0},
      {108, 162, 42, 42, 42, 42,  110, 0, 0, 0,   80, 14, -110,   0} },
    // happy — smile arches with relaxed flat brows
    { {126,  66, 120, 120, 18, 18, -108, -20, 0, 0,   80, 14,  -70,   0},
      {126,  66, 120, 120, 18, 18,  108, -20, 0, 0,   80, 14,  -70,   0} },
    // excited — taller eyes, brows raised higher
    { {132, 186, 54, 54, 54, 54, -102, 0, 0, 0,   90, 16, -130,   0},
      {132, 186, 54, 54, 54, 54,  102, 0, 0, 0,   90, 16, -130,   0} },
    // surprised — round eyes, brows way up
    { {150, 150, 75, 75, 75, 75,  -96, 0, 0, 0,   90, 14, -135,   0},
      {150, 150, 75, 75, 75, 75,   96, 0, 0, 0,   90, 14, -135,   0} },
    // thinking — left brow lower, right brow raised — asymmetric quizzical look
    { {108, 162, 42, 42, 42, 42, -114, 0, -15, 0,   80, 14, -100,   0},
      {108, 132, 42, 42, 42, 42,  114, 0,   0, -5,   80, 14, -130,   0} },
    // sad — frown arches, inner brow ends RAISED (classic sad upside-down V)
    { {126,  66, 18, 18, 120, 120, -108, 0, 10, 0,   80, 14,  -60, -15},
      {126,  66, 18, 18, 120, 120,  108, 0, 0, -10,  80, 14,  -60,  15} },
    // sleepy — collapsed slits, brows low and thin
    { {132,  24, 12, 12, 12, 12, -102, 0, 0, 0,   70, 10,  -30,   0},
      {132,  24, 12, 12, 12, 12,  102, 0, 0, 0,   70, 10,  -30,   0} },
};

// Look up an emotion id by name string (returns -1 if unknown).
inline int emotion_id_from_name(const char *name) {
    for (uint8_t i = 0; i < EMOTION_COUNT; ++i) {
        if (strcmp(name, EMOTION_NAMES[i]) == 0) return (int)i;
    }
    return -1;
}
