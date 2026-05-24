# Pip Face Firmware

Runs on the ESP32-S3-Touch-LCD-2.1. Receives `EMOTION`/`BLINK`/`RESET`/`PING`
commands over USB serial from the Pi and renders Pip's face on the 480×480
round LCD.

## Build & flash

```bash
cd firmware/face
pio run -t upload         # compile and flash via the USB (or UART) port
pio device monitor        # open serial monitor at 115200 baud
```

On boot the board prints `READY v0.7` over serial.

## Manual test from the command line

With the ESP32 plugged into the Pi:

```bash
echo "EMOTION happy #00FF00" > /dev/ttyACM0
```

The eyes should change.

## Wire protocol

**Pi → ESP32:** `EMOTION <name> <#RRGGBB>` | `BLINK` | `RESET` | `PING`

**ESP32 → Pi:** `READY v<x.y>` (on boot) | `OK` | `PONG` | `LOG <text>`

## Library choice — Arduino-GFX 1.4.9 + custom ESP-IDF display init

The plan originally called for Arduino-GFX 1.5+ (which has `Arduino_ST7701S_RGBPanel`), but that requires Arduino-ESP32 3.x. The `espressif32` PlatformIO platform ships Arduino-ESP32 2.0.17, so we're pinned to Arduino-GFX 1.4.9.

Additionally, neither Arduino-GFX nor LovyanGFX drives the **TCA9554 IO expander** on this board out of the box — the expander routes the LCD's RST and CS lines, so standard init sequences don't work.

The solution: display init uses ESP-IDF's `esp_lcd_new_rgb_panel()` directly, with the TCA9554 handled in `TCA9554PWR.cpp`. The approach is adapted from:
https://gist.github.com/fallenartist/22d1d01e125afb02ae4ebdcdf02d1f80

Arduino-GFX 1.4.9 is used only for its drawing API (`fillRoundRect`, `fillScreen`, etc.) — the panel init is entirely custom. PSRAM is allocated via `malloc()` for the software framebuffer (enabled by `-DCONFIG_SPIRAM_USE_MALLOC` in platformio.ini).

## Buzzer note

The board has a piezo buzzer on TCA9554 EXIO_PIN8. `setup()` explicitly drives it LOW to keep it silent. If you hear buzzing after handling the board, check for a cracked solder joint near the buzzer — it's a hardware issue, not a firmware bug.
