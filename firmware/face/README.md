# Pip Face Firmware

Runs on the ESP32-S3-Touch-LCD-2.1. Receives `EMOTION`/`BLINK`/`RESET`/`PING`
commands over USB serial from the Pi and renders Pip's face on the 480x480
round LCD.

## Build & flash

```bash
cd firmware/face
pio run -t upload         # compile and flash via the USB port
pio device monitor        # open serial monitor at 115200 baud
```

## Manual test from the command line

With the ESP32 plugged in, open a separate terminal:

```bash
echo "EMOTION happy #00FF00" > /dev/ttyACM0
```

The eyes should change.

## Library choice — LovyanGFX (not Arduino-GFX)

We use **LovyanGFX** to drive the ST7701S 480×480 round panel. The original plan called for Arduino-GFX, but Arduino-GFX 1.5+ (which has `Arduino_ST7701S_RGBPanel`) requires Arduino-ESP32 3.x — and the `espressif32` PlatformIO platform ships Arduino-ESP32 2.0.17. LovyanGFX 1.2 has mature ST7701S RGB support and works on Arduino-ESP32 2.x, so it's the cleaner path.

The drawing API (`fillRoundRect`, `fillScreen`, RGB565 colors) is nearly identical to Arduino-GFX, so code in later tasks adapts mechanically. The only major difference is the display init: LovyanGFX uses an `LGFX` subclass with `Bus_RGB`, `Panel_ST7701` and `Light_PWM` config structs instead of `Arduino_DataBus` + `Arduino_GFX`.
