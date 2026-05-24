// firmware/face/src/main.cpp
// Pip — serial-driven emotion dispatch.
#include <Arduino.h>
#include "Display_ST7701.h"
#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "emotions.h"
#include "face_render.h"
#include "serial_parser.h"

static uint16_t g_eye_color = 0x4DDF;
static EmotionId g_emotion = EMO_NEUTRAL;

// Convert "#RRGGBB" to RGB565. Returns false if input is malformed.
static bool hex_to_rgb565(const char *hex, uint16_t &out) {
    if (!hex || hex[0] != '#') return false;
    long v = strtol(hex + 1, nullptr, 16);
    uint8_t r = (v >> 16) & 0xFF;
    uint8_t g = (v >> 8) & 0xFF;
    uint8_t b = v & 0xFF;
    out = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return true;
}

static void render_current() {
    face_render_emotion(g_emotion, g_eye_color);
}

static void handle_command(const char *line) {
    // EMOTION <name> <#RRGGBB>
    if (strncmp(line, "EMOTION ", 8) == 0) {
        char name[16] = {0};
        char hex[8]  = {0};
        if (sscanf(line + 8, "%15s %7s", name, hex) == 2) {
            int eid = emotion_id_from_name(name);
            uint16_t color;
            if (eid >= 0 && hex_to_rgb565(hex, color)) {
                g_emotion = (EmotionId)eid;
                g_eye_color = color;
                render_current();
                Serial.println("OK");
                return;
            }
        }
        Serial.print("LOG bad_emotion: ");
        Serial.println(line);
    } else if (strcmp(line, "RESET") == 0) {
        g_emotion = EMO_NEUTRAL;
        g_eye_color = 0x4DDF;
        render_current();
        Serial.println("OK");
    } else if (strcmp(line, "PING") == 0) {
        Serial.println("PONG");
    } else if (strcmp(line, "BLINK") == 0) {
        // Animation comes in Task 14 — for now just acknowledge.
        Serial.println("OK");
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
    LCD_Init();
    Set_Backlight(50);

    if (face_render_init()) {
        render_current();
    } else {
        Serial.println("LOG face_render_init failed");
    }

    parser_setup(handle_command);
    Serial.println("READY v0.3");
}

void loop() {
    parser_poll();
    delay(5);
}
