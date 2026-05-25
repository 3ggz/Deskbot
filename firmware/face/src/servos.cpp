// firmware/face/src/servos.cpp
// Smooth-interpolated two-servo head control.
// Position is tracked in microseconds; movement uses cubic ease-in-out.
#include "servos.h"
#include <ESP32Servo.h>
#include <math.h>

static const int PAN_PIN  = 43;
static const int TILT_PIN = 44;

// SG90 pulse range (microseconds)
static const int SERVO_MIN_US = 500;     // 0°
static const int SERVO_MAX_US = 2500;    // 180°
static const int SERVO_CENTER_DEG = 90;

// Per-servo trim (mutable — adjust via SERVO_TRIM command).
static int pan_trim_deg  = 0;
static int tilt_trim_deg = 0;

// Travel limits (mutable — adjust via SERVO_LIMITS command).
static int pan_min_deg  = 30;
static int pan_max_deg  = 150;
static int tilt_min_deg = 60;
static int tilt_max_deg = 120;

// Default transition for explicit PAN/TILT commands.
static const uint32_t DEFAULT_TRANSITION_MS = 700;

static Servo pan_servo;
static Servo tilt_servo;
static bool  servos_attached = false;

// Per-axis transition state (microseconds).
static int      pan_current_us  = 1500;
static int      pan_start_us    = 1500;
static int      pan_target_us   = 1500;
static uint32_t pan_step_start_ms = 0;
static uint32_t pan_step_dur_ms   = 0;

static int      tilt_current_us  = 1500;
static int      tilt_start_us    = 1500;
static int      tilt_target_us   = 1500;
static uint32_t tilt_step_start_ms = 0;
static uint32_t tilt_step_dur_ms   = 0;

// Movement scripts.
struct Step { int pan, tilt, hold_ms; };
static const int MAX_STEPS = 12;
static Step      script[MAX_STEPS];
static int       script_len = 0;
static int       script_idx = 0;

static int clamp_i(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float ease_cubic_f(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

// Map a (clamped) angle in degrees to a pulse width in microseconds.
static int deg_to_us(int deg) {
    long us = SERVO_MIN_US + ((long)(SERVO_MAX_US - SERVO_MIN_US) * (long)deg) / 180L;
    return (int)us;
}

// Map a microsecond pulse back to an angle (for getters).
static int us_to_deg(int us) {
    long deg = ((long)(us - SERVO_MIN_US) * 180L) / (long)(SERVO_MAX_US - SERVO_MIN_US);
    if (deg < 0)   deg = 0;
    if (deg > 180) deg = 180;
    return (int)deg;
}

static void write_pan_us(int us) {
    pan_current_us = us;
    if (servos_attached) pan_servo.writeMicroseconds(us);
}

static void write_tilt_us(int us) {
    tilt_current_us = us;
    if (servos_attached) tilt_servo.writeMicroseconds(us);
}

// Start a new pan transition (in degrees, after clamping + trim).
static void start_pan_transition(int target_deg, uint32_t duration_ms, uint32_t now_ms) {
    int clamped = clamp_i(target_deg + pan_trim_deg, pan_min_deg, pan_max_deg);
    pan_start_us = pan_current_us;
    pan_target_us = deg_to_us(clamped);
    pan_step_start_ms = now_ms;
    pan_step_dur_ms   = duration_ms;
}

static void start_tilt_transition(int target_deg, uint32_t duration_ms, uint32_t now_ms) {
    int clamped = clamp_i(target_deg + tilt_trim_deg, tilt_min_deg, tilt_max_deg);
    tilt_start_us = tilt_current_us;
    tilt_target_us = deg_to_us(clamped);
    tilt_step_start_ms = now_ms;
    tilt_step_dur_ms   = duration_ms;
}

void servos_init() {
    bool pan_ok  = pan_servo.attach(PAN_PIN,  SERVO_MIN_US, SERVO_MAX_US);
    bool tilt_ok = tilt_servo.attach(TILT_PIN, SERVO_MIN_US, SERVO_MAX_US);
    servos_attached = pan_ok && tilt_ok;

    Serial.print("LOG servo_init pan_pin="); Serial.print(PAN_PIN);
    Serial.print(" tilt_pin=");              Serial.print(TILT_PIN);
    Serial.print(" pan_attached=");          Serial.print(pan_ok ? 1 : 0);
    Serial.print(" tilt_attached=");         Serial.print(tilt_ok ? 1 : 0);
    Serial.println();

    int center_us = deg_to_us(SERVO_CENTER_DEG);
    pan_current_us = pan_start_us = pan_target_us = center_us;
    tilt_current_us = tilt_start_us = tilt_target_us = center_us;
    pan_step_dur_ms = tilt_step_dur_ms = 0;
    write_pan_us(center_us);
    write_tilt_us(center_us);

    script_len = 0;
    script_idx = 0;
}

void servo_pan_to(int angle_deg) {
    script_len = 0;
    start_pan_transition(angle_deg, DEFAULT_TRANSITION_MS, millis());
}

void servo_tilt_to(int angle_deg) {
    script_len = 0;
    start_tilt_transition(angle_deg, DEFAULT_TRANSITION_MS, millis());
}

static void start_script(const Step *steps, int n, uint32_t now_ms) {
    int copy = (n > MAX_STEPS) ? MAX_STEPS : n;
    for (int i = 0; i < copy; ++i) script[i] = steps[i];
    script_len = copy;
    script_idx = 0;
    start_pan_transition(script[0].pan,  script[0].hold_ms, now_ms);
    start_tilt_transition(script[0].tilt, script[0].hold_ms, now_ms);
}

void servo_nod() {
    // Small head dip then back — center ±12° tilt, slow and gentle.
    Step s[] = {
        {SERVO_CENTER_DEG, SERVO_CENTER_DEG + 12, 600},  // small dip down
        {SERVO_CENTER_DEG, SERVO_CENTER_DEG - 8,  600},  // gentle look up
        {SERVO_CENTER_DEG, SERVO_CENTER_DEG,      500},  // return to center
    };
    start_script(s, 3, millis());
}

void servo_shake() {
    // Small "no" — pan ±20° from center, three oscillations, mellow pace.
    Step s[] = {
        {SERVO_CENTER_DEG - 20, SERVO_CENTER_DEG, 500},
        {SERVO_CENTER_DEG + 20, SERVO_CENTER_DEG, 500},
        {SERVO_CENTER_DEG - 15, SERVO_CENTER_DEG, 500},
        {SERVO_CENTER_DEG + 15, SERVO_CENTER_DEG, 500},
        {SERVO_CENTER_DEG,      SERVO_CENTER_DEG, 500},
    };
    start_script(s, 5, millis());
}

void servo_tilt_left() {
    Step s[] = {
        {SERVO_CENTER_DEG - 18, SERVO_CENTER_DEG, 700},
        {SERVO_CENTER_DEG,      SERVO_CENTER_DEG, 600},
    };
    start_script(s, 2, millis());
}

void servo_tilt_right() {
    Step s[] = {
        {SERVO_CENTER_DEG + 18, SERVO_CENTER_DEG, 700},
        {SERVO_CENTER_DEG,      SERVO_CENTER_DEG, 600},
    };
    start_script(s, 2, millis());
}

void servo_wiggle() {
    // Slower wiggle, smaller amplitude — less likely to topple.
    Step s[] = {
        {SERVO_CENTER_DEG - 12, SERVO_CENTER_DEG, 350},
        {SERVO_CENTER_DEG + 12, SERVO_CENTER_DEG, 350},
        {SERVO_CENTER_DEG - 12, SERVO_CENTER_DEG, 350},
        {SERVO_CENTER_DEG + 12, SERVO_CENTER_DEG, 350},
        {SERVO_CENTER_DEG,      SERVO_CENTER_DEG, 400},
    };
    start_script(s, 5, millis());
}

void servo_idle() {
    Step s[] = {
        {SERVO_CENTER_DEG, SERVO_CENTER_DEG, 600},
    };
    start_script(s, 1, millis());
}

void servos_tick(uint32_t now_ms) {
    // Pan interpolation
    if (pan_step_dur_ms > 0) {
        uint32_t dt = now_ms - pan_step_start_ms;
        float t = (float)dt / (float)pan_step_dur_ms;
        if (t >= 1.0f) {
            write_pan_us(pan_target_us);
        } else {
            float e = ease_cubic_f(t);
            int us = pan_start_us + (int)((float)(pan_target_us - pan_start_us) * e);
            write_pan_us(us);
        }
    }

    // Tilt interpolation
    if (tilt_step_dur_ms > 0) {
        uint32_t dt = now_ms - tilt_step_start_ms;
        float t = (float)dt / (float)tilt_step_dur_ms;
        if (t >= 1.0f) {
            write_tilt_us(tilt_target_us);
        } else {
            float e = ease_cubic_f(t);
            int us = tilt_start_us + (int)((float)(tilt_target_us - tilt_start_us) * e);
            write_tilt_us(us);
        }
    }

    // Advance script when BOTH axes have finished their current step.
    if (script_len > 0 && script_idx < script_len) {
        bool pan_done  = (now_ms - pan_step_start_ms)  >= pan_step_dur_ms;
        bool tilt_done = (now_ms - tilt_step_start_ms) >= tilt_step_dur_ms;
        if (pan_done && tilt_done) {
            script_idx++;
            if (script_idx < script_len) {
                start_pan_transition(script[script_idx].pan,   script[script_idx].hold_ms, now_ms);
                start_tilt_transition(script[script_idx].tilt, script[script_idx].hold_ms, now_ms);
            } else {
                script_len = 0;
            }
        }
    }
}

int servo_pan_current()  { return us_to_deg(pan_current_us); }
int servo_tilt_current() { return us_to_deg(tilt_current_us); }
int servo_pan_target()   { return us_to_deg(pan_target_us);  }
int servo_tilt_target()  { return us_to_deg(tilt_target_us); }

void servo_set_limits(int pan_min, int pan_max, int tilt_min, int tilt_max) {
    pan_min_deg  = clamp_i(pan_min,  0, 180);
    pan_max_deg  = clamp_i(pan_max,  0, 180);
    tilt_min_deg = clamp_i(tilt_min, 0, 180);
    tilt_max_deg = clamp_i(tilt_max, 0, 180);
    if (pan_min_deg  > pan_max_deg)  { int t = pan_min_deg;  pan_min_deg  = pan_max_deg;  pan_max_deg  = t; }
    if (tilt_min_deg > tilt_max_deg) { int t = tilt_min_deg; tilt_min_deg = tilt_max_deg; tilt_max_deg = t; }
}

void servo_set_trim(int pan_trim, int tilt_trim) {
    pan_trim_deg  = clamp_i(pan_trim,  -90, 90);
    tilt_trim_deg = clamp_i(tilt_trim, -90, 90);
}

int servo_get_pan_min()   { return pan_min_deg; }
int servo_get_pan_max()   { return pan_max_deg; }
int servo_get_tilt_min()  { return tilt_min_deg; }
int servo_get_tilt_max()  { return tilt_max_deg; }
int servo_get_pan_trim()  { return pan_trim_deg; }
int servo_get_tilt_trim() { return tilt_trim_deg; }
