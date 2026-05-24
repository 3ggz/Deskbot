// firmware/face/src/main.cpp
// Pip — render the neutral emotion as a static frame on boot.
#include <Arduino.h>
#include "Display_ST7701.h"
#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "emotions.h"
#include "face_render.h"

static const uint16_t EYE_COLOR_CYAN = 0x4DDF;  // ~#4DD0FF

void setup() {
    Serial.begin(115200);
    delay(500);

    // Bring up I2C, expander, LCD, backlight.
    I2C_Init();
    TCA9554PWR_Init(0x00);
    LCD_Init();
    Set_Backlight(50);

    if (!face_render_init()) {
        Serial.println("LOG face_render_init failed; halting render");
    } else {
        face_render_emotion(EMO_NEUTRAL, EYE_COLOR_CYAN);
    }

    Serial.println("READY v0.2");
}

void loop() {
    static uint8_t current = 0;
    static uint32_t last_change = 0;
    if (millis() - last_change > 2000) {
        current = (current + 1) % EMOTION_COUNT;
        face_render_emotion((EmotionId)current, 0x4DDF);
        Serial.print("LOG cycle: ");
        Serial.println(EMOTION_NAMES[current]);
        last_change = millis();
    }
}
