// firmware/face/src/main.cpp
// Pip — emotion+color with smooth animated transitions.
#include <Arduino.h>
#include "Display_ST7701.h"
#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "emotions.h"
#include "face_render.h"
#include "serial_parser.h"
#include "servos.h"
#include "state.h"

static FaceState g_state;

static bool hex_to_rgb565(const char *hex, uint16_t &out) {
    if (!hex || hex[0] != '#') return false;
    long v = strtol(hex + 1, nullptr, 16);
    uint8_t r = (v >> 16) & 0xFF;
    uint8_t g = (v >> 8) & 0xFF;
    uint8_t b = v & 0xFF;
    out = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return true;
}

static void handle_command(const char *line) {
    if (strncmp(line, "EMOTION ", 8) == 0) {
        char name[16] = {0};
        char hex[8]  = {0};
        if (sscanf(line + 8, "%15s %7s", name, hex) == 2) {
            int eid = emotion_id_from_name(name);
            uint16_t color;
            if (eid >= 0 && hex_to_rgb565(hex, color)) {
                state_set_target_emotion(g_state, (EmotionId)eid, color, millis());
                Serial.println("OK");
                return;
            }
        }
        Serial.print("LOG bad_emotion: ");
        Serial.println(line);
    } else if (strcmp(line, "RESET") == 0) {
        state_set_target_emotion(g_state, EMO_NEUTRAL, 0x4DDF, millis());
        Serial.println("OK");
    } else if (strcmp(line, "PING") == 0) {
        Serial.println("PONG");
    } else if (strcmp(line, "BLINK") == 0) {
        state_trigger_blink(g_state, millis());
        Serial.println("OK");
    } else if (strncmp(line, "SERVO ", 6) == 0) {
        const char *act = line + 6;
        if      (strcmp(act, "nod") == 0)         servo_nod();
        else if (strcmp(act, "shake") == 0)       servo_shake();
        else if (strcmp(act, "tilt_left") == 0)   servo_tilt_left();
        else if (strcmp(act, "tilt_right") == 0)  servo_tilt_right();
        else if (strcmp(act, "wiggle") == 0)      servo_wiggle();
        else if (strcmp(act, "idle") == 0)        servo_idle();
        else {
            Serial.print("LOG bad_servo: ");
            Serial.println(act);
            return;
        }
        Serial.print("LOG servo_cmd="); Serial.print(act);
        Serial.print(" pan_now="); Serial.print(servo_pan_current());
        Serial.print(" tilt_now="); Serial.print(servo_tilt_current());
        Serial.println();
        Serial.println("OK");
        return;
    } else if (strcmp(line, "SERVO_TEST_HIGH") == 0) {
        // Bypass LEDC — toggle the pan pin to plain HIGH so the user can
        // verify with a multimeter that GPIO43 actually reaches ~3.3V.
        ledcDetachPin(43);
        ledcDetachPin(44);
        pinMode(43, OUTPUT); digitalWrite(43, HIGH);
        pinMode(44, OUTPUT); digitalWrite(44, HIGH);
        Serial.println("LOG SERVO_TEST_HIGH pins 43,44 set HIGH");
        Serial.println("OK");
        return;
    } else if (strcmp(line, "SERVO_TEST_LOW") == 0) {
        ledcDetachPin(43);
        ledcDetachPin(44);
        pinMode(43, OUTPUT); digitalWrite(43, LOW);
        pinMode(44, OUTPUT); digitalWrite(44, LOW);
        Serial.println("LOG SERVO_TEST_LOW pins 43,44 set LOW");
        Serial.println("OK");
        return;
    } else if (strcmp(line, "SERVO_RESET") == 0) {
        servos_init();
        Serial.println("OK");
        return;
    } else {
        Serial.print("LOG bad_cmd: ");
        Serial.println(line);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    I2C_Init();
    TCA9554PWR_Init(0x00);
    // EXIO_PIN8 is the onboard piezo BUZZER on this board — explicitly hold LOW to keep it silent.
    Set_EXIO(EXIO_PIN8, Low);
    LCD_Init();
    Set_Backlight(50);

    if (!face_render_init()) {
        Serial.println("LOG face_render_init failed");
    }
    state_init(g_state);
    servos_init();
    parser_setup(handle_command);
    Serial.println("READY v0.9-diag");
}

void loop() {
    parser_poll();
    state_update_animation(g_state, millis());
    servos_tick(millis());
    int gx = state_current_pupil_offset(g_state, millis());
    face_render_state(g_state.current, g_state.current_color, gx);
    delay(16);  // ~60 FPS
}
