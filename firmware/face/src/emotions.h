// firmware/face/src/emotions.h
// 12 emotion shapes for Pip's face. Values from the design spec
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
    EMO_ANGRY,
    EMO_LOVE,
    EMO_CONFUSED,
    EMO_EMBARRASSED,
    EMO_WINK,
    EMOTION_COUNT
};

static const char* EMOTION_NAMES[EMOTION_COUNT] = {
    "neutral", "happy", "excited", "surprised", "thinking", "sad", "sleepy",
    "angry", "love", "confused", "embarrassed", "wink"
};

// Symmetric pair: index [emo].left and [emo].right
struct EyePair { EyeShape left; EyeShape right; };

static const EyePair EMOTIONS[EMOTION_COUNT] = {
    // neutral — default rounded bars, flat brows above
    { {108, 162, 42, 42, 42, 42, -110, 0, 0, 0,   80, 14, -110,   0},
      {108, 162, 42, 42, 42, 42,  110, 0, 0, 0,   80, 14, -110,   0} },

    // happy — smile arches; brows lifted relaxed-high, slight outer-up tilt (warm/friendly)
    { {126,  66, 120, 120, 18, 18, -108, -20, 0, 0,   86, 14, -125,  -6},
      {126,  66, 120, 120, 18, 18,  108, -20, 0, 0,   86, 14, -125,   6} },

    // excited — taller eyes, brows raised higher with slight excited tilt
    { {132, 186, 54, 54, 54, 54, -102, 0, 0, 0,   90, 16, -135,  -3},
      {132, 186, 54, 54, 54, 54,  102, 0, 0, 0,   90, 16, -135,   3} },

    // surprised — round eyes, brows way up + wider
    { {150, 150, 75, 75, 75, 75,  -96, 0, 0, 0,  100, 14, -145,   0},
      {150, 150, 75, 75, 75, 75,   96, 0, 0, 0,  100, 14, -145,   0} },

    // thinking — left brow noticeably lower, right brow exaggerated higher (clearly asymmetric)
    { {108, 162, 42, 42, 42, 42, -114, 0, -15, 0,   80, 14,  -95,   0},
      {108, 132, 42, 42, 42, 42,  114, 0,   0, -5,  80, 14, -145,   0} },

    // sad — frown arches, inner brow ends RAISED (upside-down V)
    { {126,  66, 18, 18, 120, 120, -108, 0, 10, 0,   84, 14,  -75, -18},
      {126,  66, 18, 18, 120, 120,  108, 0, 0, -10,  84, 14,  -75,  18} },

    // sleepy — collapsed slits, brows low/thin/relaxed
    { {132,  24, 12, 12, 12, 12, -102, 0, 0, 0,   70, 10,  -50,   0},
      {132,  24, 12, 12, 12, 12,  102, 0, 0, 0,   70, 10,  -50,   0} },

    // angry — narrowed eyes; brows angled INWARD-DOWN (opposite of sad — inner edges LOW)
    { {120,  90, 30, 30, 30, 30, -108, 5, -8, 0,   88, 16,  -85,  18},
      {120,  90, 30, 30, 30, 30,  108, 5, 0, 8,    88, 16,  -85, -18} },

    // love — wide bright eyes (slightly rounder than surprised), brows lifted gently
    { {138, 138, 70, 70, 70, 70, -100, -8, 0, 0,   84, 14, -130,  -8},
      {138, 138, 70, 70, 70, 70,  100, -8, 0, 0,   84, 14, -130,   8} },

    // confused — one brow MUCH higher than the other (more extreme than thinking)
    { {108, 152, 42, 42, 42, 42, -114, 0,  5, 0,   82, 14,  -75,   8},
      {108, 132, 42, 42, 42, 42,  114, 0,  0, 0,   82, 14, -155, -10} },

    // embarrassed — smaller eyes looking slightly down; brows lifted "innocently"
    { { 96, 130, 38, 38, 38, 38, -106, 12, 0, 0,   82, 14, -120,  -8},
      { 96, 130, 38, 38, 38, 38,  106, 12, 0, 0,   82, 14, -120,   8} },

    // wink — LEFT eye collapsed to slit (winking), RIGHT eye normal-happy
    { {126,  16, 8,  8,  8,  8,  -108, 0,  0, 0,   86, 14, -125,  -6},
      {126,  66, 120,120,18, 18,  108, -20,0, 0,   86, 14, -125,   6} },
};

// Look up an emotion id by name string (returns -1 if unknown).
inline int emotion_id_from_name(const char *name) {
    for (uint8_t i = 0; i < EMOTION_COUNT; ++i) {
        if (strcmp(name, EMOTION_NAMES[i]) == 0) return (int)i;
    }
    return -1;
}
