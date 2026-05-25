// firmware/face/src/servos.cpp
#include "servos.h"

// GPIO assignments
static const int PAN_PIN  = 43;
static const int TILT_PIN = 44;

// LEDC channels and config
static const int PAN_CHANNEL  = 4;   // channels 0-3 are used by LCD backlight + reserved
static const int TILT_CHANNEL = 5;
static const int SERVO_FREQ_HZ   = 50;
static const int SERVO_RESOLUTION = 16;  // 16-bit duty resolution

// SG90 calibration (typical)
static const int SERVO_MIN_US = 500;    // 0°
static const int SERVO_MAX_US = 2500;   // 180°
static const int SERVO_CENTER_DEG = 90;

// Per-servo trim — adjust if your mounting is off-center.
// Positive = bias forward / right of physical center.
static const int PAN_TRIM_DEG  = 0;
static const int TILT_TRIM_DEG = 0;

// Travel limits to avoid binding against the mount.
static const int PAN_MIN_DEG  = 30;
static const int PAN_MAX_DEG  = 150;
static const int TILT_MIN_DEG = 60;
static const int TILT_MAX_DEG = 120;

// Current and target angles
static int  pan_current_deg  = SERVO_CENTER_DEG;
static int  tilt_current_deg = SERVO_CENTER_DEG;
static int  pan_target_deg   = SERVO_CENTER_DEG;
static int  tilt_target_deg  = SERVO_CENTER_DEG;

// Movement scripts (sequence of (pan, tilt, hold_ms) steps).
struct Step { int pan, tilt, hold_ms; };
static const int MAX_STEPS = 12;
static Step      script[MAX_STEPS];
static int       script_len = 0;
static int       script_idx = 0;
static uint32_t  script_step_start_ms = 0;

static int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static uint32_t deg_to_duty(int deg) {
    // Map degree → microseconds → 16-bit duty (period = 1e6 / 50 = 20000 us).
    int us = SERVO_MIN_US + (int)(((long)(SERVO_MAX_US - SERVO_MIN_US) * deg) / 180L);
    // duty = us * (2^16 - 1) / 20000
    return (uint32_t)(((long)us * 65535L) / 20000L);
}

static void write_pan_raw(int deg) {
    int d = clamp(deg + PAN_TRIM_DEG, PAN_MIN_DEG, PAN_MAX_DEG);
    ledcWrite(PAN_CHANNEL, deg_to_duty(d));
    pan_current_deg = d;
}

static void write_tilt_raw(int deg) {
    int d = clamp(deg + TILT_TRIM_DEG, TILT_MIN_DEG, TILT_MAX_DEG);
    ledcWrite(TILT_CHANNEL, deg_to_duty(d));
    tilt_current_deg = d;
}

void servos_init() {
    // Force GPIO mode before LEDC attach. Arduino-ESP32 sometimes leaves these
    // as UART0 even when Serial1 isn't started.
    pinMode(PAN_PIN,  OUTPUT);
    pinMode(TILT_PIN, OUTPUT);
    digitalWrite(PAN_PIN,  LOW);
    digitalWrite(TILT_PIN, LOW);

    bool pan_ok  = ledcSetup(PAN_CHANNEL,  SERVO_FREQ_HZ, SERVO_RESOLUTION) > 0;
    ledcAttachPin(PAN_PIN,  PAN_CHANNEL);
    bool tilt_ok = ledcSetup(TILT_CHANNEL, SERVO_FREQ_HZ, SERVO_RESOLUTION) > 0;
    ledcAttachPin(TILT_PIN, TILT_CHANNEL);

    Serial.print("LOG servo_init pan_pin="); Serial.print(PAN_PIN);
    Serial.print(" tilt_pin=");              Serial.print(TILT_PIN);
    Serial.print(" pan_ledc_ok=");           Serial.print(pan_ok ? 1 : 0);
    Serial.print(" tilt_ledc_ok=");          Serial.print(tilt_ok ? 1 : 0);
    Serial.println();

    write_pan_raw(SERVO_CENTER_DEG);
    write_tilt_raw(SERVO_CENTER_DEG);
    pan_target_deg  = SERVO_CENTER_DEG;
    tilt_target_deg = SERVO_CENTER_DEG;
    script_len = 0;
    script_idx = 0;

    uint32_t center_duty = deg_to_duty(SERVO_CENTER_DEG);
    Serial.print("LOG servo_center_duty=");  Serial.println(center_duty);
}

int servo_pan_current()  { return pan_current_deg; }
int servo_tilt_current() { return tilt_current_deg; }

void servo_pan_to(int angle_deg) {
    pan_target_deg = clamp(angle_deg, PAN_MIN_DEG, PAN_MAX_DEG);
}

void servo_tilt_to(int angle_deg) {
    tilt_target_deg = clamp(angle_deg, TILT_MIN_DEG, TILT_MAX_DEG);
}

static void start_script(const Step *steps, int n, uint32_t now_ms) {
    int copy = (n > MAX_STEPS) ? MAX_STEPS : n;
    for (int i = 0; i < copy; ++i) script[i] = steps[i];
    script_len = copy;
    script_idx = 0;
    script_step_start_ms = now_ms;
    pan_target_deg  = script[0].pan;
    tilt_target_deg = script[0].tilt;
}

void servo_nod() {
    // Yes: tilt down, tilt up, return
    static const Step s[] = {
        {SERVO_CENTER_DEG, TILT_MAX_DEG, 300},
        {SERVO_CENTER_DEG, TILT_MIN_DEG, 300},
        {SERVO_CENTER_DEG, SERVO_CENTER_DEG, 200},
    };
    start_script(s, 3, millis());
}

void servo_shake() {
    // No: pan left, pan right, return
    static const Step s[] = {
        {PAN_MIN_DEG, SERVO_CENTER_DEG, 250},
        {PAN_MAX_DEG, SERVO_CENTER_DEG, 250},
        {PAN_MIN_DEG, SERVO_CENTER_DEG, 250},
        {PAN_MAX_DEG, SERVO_CENTER_DEG, 250},
        {SERVO_CENTER_DEG, SERVO_CENTER_DEG, 200},
    };
    start_script(s, 5, millis());
}

void servo_tilt_left() {
    static const Step s[] = {
        {SERVO_CENTER_DEG - 30, SERVO_CENTER_DEG, 400},
        {SERVO_CENTER_DEG,      SERVO_CENTER_DEG, 200},
    };
    start_script(s, 2, millis());
}

void servo_tilt_right() {
    static const Step s[] = {
        {SERVO_CENTER_DEG + 30, SERVO_CENTER_DEG, 400},
        {SERVO_CENTER_DEG,      SERVO_CENTER_DEG, 200},
    };
    start_script(s, 2, millis());
}

void servo_wiggle() {
    static const Step s[] = {
        {SERVO_CENTER_DEG - 20, SERVO_CENTER_DEG, 150},
        {SERVO_CENTER_DEG + 20, SERVO_CENTER_DEG, 150},
        {SERVO_CENTER_DEG - 20, SERVO_CENTER_DEG, 150},
        {SERVO_CENTER_DEG + 20, SERVO_CENTER_DEG, 150},
        {SERVO_CENTER_DEG,      SERVO_CENTER_DEG, 200},
    };
    start_script(s, 5, millis());
}

void servo_idle() {
    static const Step s[] = {
        {SERVO_CENTER_DEG, SERVO_CENTER_DEG, 200},
    };
    start_script(s, 1, millis());
}

void servos_tick(uint32_t now_ms) {
    // Smoothly move current angles toward target (1 deg per tick — comes out
    // to roughly 60 deg/sec at 60 FPS; tune by upping the step if too slow).
    if (pan_current_deg < pan_target_deg)       write_pan_raw(pan_current_deg + 1);
    else if (pan_current_deg > pan_target_deg)  write_pan_raw(pan_current_deg - 1);

    if (tilt_current_deg < tilt_target_deg)      write_tilt_raw(tilt_current_deg + 1);
    else if (tilt_current_deg > tilt_target_deg) write_tilt_raw(tilt_current_deg - 1);

    // Advance script if step's hold time has elapsed AND we've reached the target.
    if (script_len > 0 && script_idx < script_len) {
        bool at_target = (pan_current_deg == pan_target_deg && tilt_current_deg == tilt_target_deg);
        if (at_target && (now_ms - script_step_start_ms) >= (uint32_t)script[script_idx].hold_ms) {
            script_idx++;
            if (script_idx < script_len) {
                pan_target_deg  = script[script_idx].pan;
                tilt_target_deg = script[script_idx].tilt;
                script_step_start_ms = now_ms;
            } else {
                script_len = 0;  // done
            }
        }
    }
}
