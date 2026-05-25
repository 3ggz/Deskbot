// firmware/face/src/servos.cpp
// Two-servo head control via ESP32Servo (MCPWM-backed).
#include "servos.h"
#include <ESP32Servo.h>

static const int PAN_PIN  = 43;
static const int TILT_PIN = 44;

// SG90 calibration (typical)
static const int SERVO_MIN_US = 500;    // 0°
static const int SERVO_MAX_US = 2500;   // 180°
static const int SERVO_CENTER_DEG = 90;

// Per-servo trim — adjust if mounting is off-center.
static const int PAN_TRIM_DEG  = 0;
static const int TILT_TRIM_DEG = 0;

// Travel limits to avoid binding against the mount.
static const int PAN_MIN_DEG  = 30;
static const int PAN_MAX_DEG  = 150;
static const int TILT_MIN_DEG = 60;
static const int TILT_MAX_DEG = 120;

static Servo pan_servo;
static Servo tilt_servo;
static bool  servos_attached = false;

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

static void write_pan_raw(int deg) {
    int d = clamp(deg + PAN_TRIM_DEG, PAN_MIN_DEG, PAN_MAX_DEG);
    if (servos_attached) pan_servo.write(d);
    pan_current_deg = d;
}

static void write_tilt_raw(int deg) {
    int d = clamp(deg + TILT_TRIM_DEG, TILT_MIN_DEG, TILT_MAX_DEG);
    if (servos_attached) tilt_servo.write(d);
    tilt_current_deg = d;
}

void servos_init() {
    // ESP32Servo uses the first available 50Hz timer of MCPWM by default.
    // No explicit pinMode needed — the library handles pin setup itself.
    bool pan_ok  = pan_servo.attach(PAN_PIN, SERVO_MIN_US, SERVO_MAX_US);
    bool tilt_ok = tilt_servo.attach(TILT_PIN, SERVO_MIN_US, SERVO_MAX_US);
    servos_attached = pan_ok && tilt_ok;

    Serial.print("LOG servo_init pan_pin="); Serial.print(PAN_PIN);
    Serial.print(" tilt_pin=");              Serial.print(TILT_PIN);
    Serial.print(" pan_attached=");          Serial.print(pan_ok ? 1 : 0);
    Serial.print(" tilt_attached=");         Serial.print(tilt_ok ? 1 : 0);
    Serial.println();

    write_pan_raw(SERVO_CENTER_DEG);
    write_tilt_raw(SERVO_CENTER_DEG);
    pan_target_deg  = SERVO_CENTER_DEG;
    tilt_target_deg = SERVO_CENTER_DEG;
    script_len = 0;
    script_idx = 0;
}

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
    static const Step s[] = {
        {SERVO_CENTER_DEG, TILT_MAX_DEG, 300},
        {SERVO_CENTER_DEG, TILT_MIN_DEG, 300},
        {SERVO_CENTER_DEG, SERVO_CENTER_DEG, 200},
    };
    start_script(s, 3, millis());
}

void servo_shake() {
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
    if (pan_current_deg < pan_target_deg)       write_pan_raw(pan_current_deg + 1);
    else if (pan_current_deg > pan_target_deg)  write_pan_raw(pan_current_deg - 1);

    if (tilt_current_deg < tilt_target_deg)      write_tilt_raw(tilt_current_deg + 1);
    else if (tilt_current_deg > tilt_target_deg) write_tilt_raw(tilt_current_deg - 1);

    if (script_len > 0 && script_idx < script_len) {
        bool at_target = (pan_current_deg == pan_target_deg && tilt_current_deg == tilt_target_deg);
        if (at_target && (now_ms - script_step_start_ms) >= (uint32_t)script[script_idx].hold_ms) {
            script_idx++;
            if (script_idx < script_len) {
                pan_target_deg  = script[script_idx].pan;
                tilt_target_deg = script[script_idx].tilt;
                script_step_start_ms = now_ms;
            } else {
                script_len = 0;
            }
        }
    }
}

int servo_pan_current()  { return pan_current_deg; }
int servo_tilt_current() { return tilt_current_deg; }
