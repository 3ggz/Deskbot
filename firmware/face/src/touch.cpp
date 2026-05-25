// firmware/face/src/touch.cpp
#include "touch.h"
#include <Wire.h>

static const uint8_t CST816_ADDR = 0x15;

void touch_init() {
    // I2C is already initialized by I2C_Init() in main. CST816 needs no extra config.
}

bool touch_poll(int *out_x, int *out_y) {
    // Read 7 bytes starting at register 0x00
    Wire.beginTransmission(CST816_ADDR);
    Wire.write(0x00);
    if (Wire.endTransmission(false) != 0) return false;  // 'false' = restart

    int count = Wire.requestFrom((int)CST816_ADDR, 7);
    if (count < 7) {
        // drain
        while (Wire.available()) Wire.read();
        return false;
    }
    uint8_t b[7];
    for (int i = 0; i < 7; ++i) b[i] = Wire.read();

    // Layout (CST816 family):
    //  b[0]: gesture id
    //  b[1]: not-used / reserved
    //  b[2]: number of touches (1 if touched, 0 if not)
    //  b[3]: high nibble of X | event in upper bits
    //  b[4]: low byte of X
    //  b[5]: high nibble of Y | id in upper bits
    //  b[6]: low byte of Y
    uint8_t touches = b[2] & 0x0F;
    if (touches == 0) return false;

    if (out_x) *out_x = ((b[3] & 0x0F) << 8) | b[4];
    if (out_y) *out_y = ((b[5] & 0x0F) << 8) | b[6];
    return true;
}
