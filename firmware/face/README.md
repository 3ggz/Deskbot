# Pip Face Firmware

Runs on the **Waveshare ESP32-S3-Touch-LCD-2.1**. Receives commands over USB serial from the Pi and drives Pip's animated 480×480 round face, two SG90 head servos, and a capacitive touch sensor.

Current firmware version: **READY v2.4** (printed on boot).

## Build & flash

```bash
cd firmware/face
pio run -t upload         # compile and flash via the UART port (Windows build machine)
pio device monitor        # open serial monitor at 115200 baud
```

On boot the board prints `READY v2.4` over serial.

## Manual test from the command line

With the ESP32 plugged into the Pi via the USB (native OTG) port:

```bash
echo "EMOTION happy #00FF00" > /dev/ttyACM0
echo "SERVO nod" > /dev/ttyACM0
echo "TEXT Hello there!" > /dev/ttyACM0
```

## Wire protocol

### Pi → ESP32

| Command | Effect |
|---|---|
| `EMOTION <name> <#RRGGBB>` | Set emotion + eye color. Shape auto-returns to neutral after 4.5s; color persists. |
| `BLINK` | Trigger a single blink |
| `RESET` | Neutral shape + cyan color, servos to center |
| `PING` | Heartbeat — expects `PONG` |
| `TEXT <message>` | Show caption (word-wrapped, paginated every 3.5s, 4.5s default hold) |
| `TEXT_LIVE <message>` | Show caption held for 60s (live-typing mode) |
| `TEXT_CLEAR` | Dismiss caption immediately |
| `SERVO <action>` | Run named movement (see list below) |
| `PAN <deg>` | Move pan servo to absolute angle; cancels any active script |
| `TILT <deg>` | Move tilt servo to absolute angle; cancels any active script |
| `SERVO_LIMITS <pmin> <pmax> <tmin> <tmax>` | Live-tune travel limits |
| `SERVO_TRIM <ptrim> <ttrim>` | Live-tune center trim |
| `SERVO_INFO` | Print current servo state |
| `SERVO_TEST_HIGH` / `SERVO_TEST_LOW` / `SERVO_RESET` | Diagnostic GPIO raw toggle / reinit |

**Valid emotion names:** `neutral` `happy` `excited` `surprised` `thinking` `sad` `sleepy` `angry` `love` `confused` `embarrassed` `wink`

**Valid SERVO actions:** `nod` `shake` `tilt_left` `tilt_right` `wiggle` `idle` `dance` `look_around` `bow` (and the hidden `dance_crazy`)

### ESP32 → Pi

| Response | Meaning |
|---|---|
| `READY v2.4` | Boot complete |
| `OK` | Command accepted |
| `PONG` | Response to PING |
| `LOG <text>` | Diagnostic message |
| `LOG TOUCH <x>,<y>` | Touch event coordinates |

## Subsystems

### Face / Emotions (emotions.h, state.h, face_render.h)

12 emotions, each with independently tuned eye shapes, eyebrow shapes, and pupil behavior. All values interpolate with cubic-ease (300ms shape, 400ms color). Pupils are vertical ellipses in a lighter shade of the eye color; they auto-hide when eye height < 30px.

Idle state machine: ACTIVE → IDLE_BLINK (5s) → IDLE_GLANCE (35s) → IDLE_BREATHE (95s) → SLEEPY (215s). Blinks every 6–9s. Glances every 8–15s (3-phase: rise/hold/return). Always-on ±5px pupil drift. Breathing ±5% in IDLE_BREATHE and SLEEPY.

Auto-return: 4s after an EMOTION command, eye shape eases back to neutral. Color stays.

### Captions (state.h)

Multi-line word-wrap (40 chars/line, up to 12 lines). Paginated every 3.5s. Displayed as a translucent 2-line box at y=295. TEXT holds 4.5s; TEXT_LIVE holds 60s.

### Servos (servos.h, servos.cpp)

Pan on GPIO43, tilt on GPIO44. ESP32Servo library (MCPWM peripheral). Float-precision cubic-ease interpolation. Default limits: pan 30–150°, tilt 60–120°, center 90°. Ambient drift after 30s idle. Script cancellation on direct PAN/TILT command.

### Touch (touch.h, touch.cpp)

CST816 capacitive touch polled at 25Hz via I²C. Rising edge triggers one of 6 random flinch variants (emotion + caption + servo recoil). 1.5s cooldown between pokes.

## Library choice — Arduino-GFX 1.4.9 + custom ESP-IDF display init

The plan originally called for Arduino-GFX 1.5+ (which has `Arduino_ST7701S_RGBPanel`), but that requires Arduino-ESP32 3.x. The `espressif32` PlatformIO platform ships Arduino-ESP32 2.0.17, so we're pinned to Arduino-GFX 1.4.9.

Neither Arduino-GFX nor LovyanGFX drives the **TCA9554 IO expander** on this board out of the box — the expander routes the LCD's RST and CS lines, so standard init sequences don't work.

The solution: display init uses ESP-IDF's `esp_lcd_new_rgb_panel()` directly, with the TCA9554 handled in `TCA9554PWR.cpp`. Adapted from:
https://gist.github.com/fallenartist/22d1d01e125afb02ae4ebdcdf02d1f80

Arduino-GFX 1.4.9 is used only for its drawing API (`fillRoundRect`, `fillScreen`, etc.) — the panel init is entirely custom. PSRAM is allocated via `malloc()` for the software framebuffer (enabled by `-DCONFIG_SPIRAM_USE_MALLOC` in platformio.ini).

## Buzzer note

The board has a piezo buzzer on TCA9554 EXIO_PIN8. `setup()` explicitly drives it LOW to keep it silent. If you hear buzzing after handling the board (especially after a drop), check for a cracked solder joint near the buzzer — it's a hardware issue, not a firmware bug.
