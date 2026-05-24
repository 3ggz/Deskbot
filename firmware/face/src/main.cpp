// firmware/face/src/main.cpp
// Pip — Hello LCD: fill screen cyan, print READY on serial.
//
// Init order matters:
//   1. I2C_Init()         — bring up Wire so we can talk to TCA9554
//   2. TCA9554PWR_Init()  — configure the IO expander (all pins output)
//   3. LCD_Init()         — Reset via EXIO_PIN1, SPI ST7701S init, then RGB panel
//   4. Set_Backlight()    — PWM on GPIO6
//
// LCD_Init() in Display_ST7701.cpp does NOT call Touch_Init() in this build;
// that line was removed from the driver because Task 8 is display-only.

#include <Arduino.h>
#include "Display_ST7701.h"
#include "I2C_Driver.h"
#include "TCA9554PWR.h"

// Fill the entire 480x480 screen with one RGB565 color.
// LCD_addWindow() wraps esp_lcd_panel_draw_bitmap(), which takes uint8_t*.
// We allocate a single-row scratch buffer in PSRAM to keep stack usage low.
static void LCD_FillScreen(uint16_t color) {
    const uint16_t W = ESP_PANEL_LCD_WIDTH;
    const uint16_t H = ESP_PANEL_LCD_HEIGHT;

    // One row of pixels in PSRAM
    uint16_t *row = (uint16_t *)heap_caps_malloc(W * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!row) {
        Serial.println("ERROR: PSRAM alloc failed for LCD_FillScreen");
        return;
    }
    for (uint16_t i = 0; i < W; i++) row[i] = color;

    for (uint16_t y = 0; y < H; y++) {
        LCD_addWindow(0, y, W - 1, y, (uint8_t *)row);
    }
    heap_caps_free(row);
}

void setup() {
    Serial.begin(115200);
    delay(500);

    // Step 1: I2C bus (SDA=15, SCL=7)
    I2C_Init();

    // Step 2: TCA9554 IO expander — all 7 pins set to output mode (0x00)
    TCA9554PWR_Init(0x00);

    // Step 3: ST7701S + RGB panel init (Reset via EXIO_PIN1, CS via EXIO_PIN3)
    LCD_Init();

    // Step 4: Backlight at 50%
    Set_Backlight(50);

    // Fill screen cyan (RGB565 0x4DDF ≈ #4DD8FF)
    LCD_FillScreen(0x4DDF);

    Serial.println("READY v0.1");
}

void loop() {
    delay(1000);
}
