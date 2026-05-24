# Stage 2 — The Face

**Status:** Design approved, awaiting implementation plan
**Date:** 2026-05-23
**Stage:** 2 of 6 in the Desk Companion Bot build (per AGENTS.md)

---

## Goal

Give Pip a visible face. When the AI returns an emotion in its JSON response, the round display shows eyes that match — happy, sad, excited, thinking, surprised, sleepy, or neutral — and the face hue matches the `led_color` field. Between conversations the face stays alive with blinks, glances, breathing motion, and an eventual sleepy drift.

**Success criteria:**
1. Running `python3 brain.py` and typing a message makes the eyes on the round display visibly change to match the AI's chosen emotion within ~500ms of the response arriving.
2. The face color crossfades to match `led_color` from the response.
3. With no input for several minutes, the face transitions through idle states (active → blinking → glancing → breathing → sleepy) on its own.
4. If the face hardware is unplugged or unresponsive, `brain.py` continues working in terminal-only mode without crashing.

---

## Hardware

- **Brain:** Raspberry Pi 5 8GB (existing).
- **Face:** Waveshare **ESP32-S3-Touch-LCD-2.1** — integrated dev board with ESP32-S3 microcontroller + 2.1" round 480×480 ST7701S LCD + CST816S capacitive touch.
- **Link:** USB-C cable from Pi to the ESP32's "USB" port (native USB-OTG on the S3, enumerates as `/dev/ttyACM0` on Linux).

### Why this architecture (vs. driving the LCD from the Pi directly)

The ST7701S at 480×480 needs QSPI or parallel RGB to hit usable framerates — neither is cleanly broken out on the Pi 5 GPIO header. The ESP32-S3 already has the native bus to drive this panel, so we use the ESP32 as a dedicated face co-processor. Bonus: the face animates independently of the Pi's workload (it never freezes during API calls), and we get capacitive touch and WiFi/BT on the ESP32 for free.

### Why two USB-C ports on the ESP32 board (FYI, informational)

The board has two USB-C connectors. **USB** goes to the ESP32-S3's native USB-OTG controller (built into the chip; modern, fast, supports CDC-ACM serial without a bridge chip). **UART** goes through a separate USB-to-UART bridge chip wired to UART0 (legacy programming path, used by `esptool` bootloader). We use the **USB** port for our Pi link.

---

## Visual Design — The Face Itself

A hybrid of two robot-face traditions: **Cozmo/Anki Vector** (glowing rounded-rectangle eyes that change shape per emotion) and **Eve/WALL-E** (minimalist eye slits whose angle and proportion communicate everything). Two glowing primitives, no mouth, no eyebrows — the entire vocabulary is the eyes morphing through four properties: **size, border-radius, position, tilt**.

### Emotion Shape Table

All values assume a 480×480 canvas. Eye positions are symmetric about the vertical centerline. `radius_*` corners are clockwise from top-left.

| Emotion   | Width | Height | Radius (TL, TR, BR, BL) | X-offset (from center) | Y-offset | Tilt (L, R) | Notes |
|-----------|-------|--------|--------------------------|------------------------|----------|-------------|-------|
| neutral   | 108   | 162    | 42, 42, 42, 42           | ±110                   | 0        | 0°, 0°      | Default rounded vertical bars |
| happy     | 126   | 66     | 120, 120, 18, 18         | ±108                   | -20      | 0°, 0°      | Squint/smile arches |
| excited   | 132   | 186    | 54, 54, 54, 54           | ±102                   | 0        | 0°, 0°      | Taller, wider, +30% glow intensity |
| surprised | 150   | 150    | 75, 75, 75, 75           | ±96                    | 0        | 0°, 0°      | Round wide-open eyes |
| thinking  | 108 / 108 | 162 / 132 | 42, 42, 42, 42       | ±114                   | 0        | -15°, -5°   | Asymmetric tilt; right eye also shorter |
| sad       | 126   | 66     | 18, 18, 120, 120         | ±108                   | 0        | +10°, -10°  | Inverted arches, tilted inward |
| sleepy    | 132   | 24     | 12, 12, 12, 12           | ±102                   | 0        | 0°, 0°      | Collapsed slits, reduced glow |

A **blink** is a brief animated transition: shrink height to ~10px over 80ms, hold 40ms, return to current emotion's height over 80ms. Triggered every 3–5s during idle states.

### Color

Face hue follows the `led_color` field of every AI response, looked up via `config.LED_COLORS`. The face does not have its own emotion-to-color map — it trusts the AI's color choice. Color crossfade is ~400ms cubic-ease.

---

## Communication — Wire Protocol

Line-based ASCII over USB serial at **115200 baud**, `\n` terminated. Choosing ASCII over binary for debuggability — you can `screen /dev/ttyACM0 115200` and drive the face by hand for testing.

### Pi → ESP32

| Command | Example | Meaning |
|---|---|---|
| `EMOTION <name> <hex>` | `EMOTION excited #FFD700` | Set target emotion + face hue. ESP32 animates the transition. |
| `BLINK` | `BLINK` | Force an immediate blink (used for acknowledgment moments). |
| `RESET` | `RESET` | Re-center to neutral. Clears any in-flight animation. |
| `PING` | `PING` | Heartbeat; expect `PONG`. |

### ESP32 → Pi

| Command | Example | Meaning |
|---|---|---|
| `READY <version>` | `READY v1.0` | Sent once at boot. Pi uses this as a sync signal. |
| `OK` | `OK` | Acknowledgment of the last command. |
| `PONG` | `PONG` | Heartbeat reply. |
| `LOG <text>` | `LOG bad_cmd: FOO` | Free-form diagnostic. Pi logs at INFO. |
| `TOUCH <x>,<y>` | `TOUCH 240,180` | (Stretch — Stage 2.5) Capacitive touch event. |

### Resync protocol

If the ESP32 reboots (e.g., user re-plugs USB), it sends `READY` again. The Pi-side `Face` class listens for `READY` and re-sends the current emotion + color so the face resyncs without needing the Pi process to restart.

---

## ESP32 Firmware Design

### Tooling

- **PlatformIO** (VS Code extension) — handles toolchain, library deps, flashing, and serial monitor.
- **Platform:** `espressif32`
- **Framework:** `arduino`
- **Board:** `esp32-s3-devkitc-1` (or a Waveshare-specific profile if available in PIO registry)

### Libraries

- **`moononournation/GFX_Library_for_Arduino`** (Arduino-GFX) — supports ST7701S, has `fillRoundRect()` and rotation, lightweight, no GUI framework overhead. Chosen over LVGL because Pip's face is geometric primitives, not widgets.

### File layout

```
firmware/face/
├── platformio.ini
├── src/
│   ├── main.cpp       # setup(), loop(), serial parser
│   ├── face.h/.cpp    # render_frame(): draws current eye state to LCD
│   ├── emotions.h     # static EyeShape table (the data from the Emotion Shape Table above)
│   └── state.h/.cpp   # current emotion, target emotion, color, idle timers, blink scheduler
└── README.md          # How to build, flash, and serial-test
```

### Core data types

```cpp
struct EyeShape {
    int16_t width, height;
    int16_t radius_tl, radius_tr, radius_br, radius_bl;
    int16_t x_offset;     // from center, eye-pair is symmetric
    int16_t y_offset;
    int8_t  tilt_left;    // degrees
    int8_t  tilt_right;
};

struct FaceState {
    EyeShape current;     // interpolated, drawn this frame
    EyeShape target;      // what we're animating toward
    uint16_t current_color;   // RGB565
    uint16_t target_color;
    uint32_t transition_start_ms;
    uint32_t last_cmd_ms;     // for idle state transitions
    enum { ACTIVE, IDLE_BLINK, IDLE_GLANCE, IDLE_BREATHE, SLEEPY } mode;
};
```

### State machine

Single-threaded, runs in `loop()` at ~30 FPS. Time-based transitions:

| From                | Trigger                       | To              |
|---------------------|-------------------------------|-----------------|
| any                 | `EMOTION` command received    | `ACTIVE`        |
| `ACTIVE`            | 5s elapsed since last command | `IDLE_BLINK`    |
| `IDLE_BLINK`        | 30s in this state             | `IDLE_GLANCE`   |
| `IDLE_GLANCE`       | 60s in this state             | `IDLE_BREATHE`  |
| `IDLE_BREATHE`      | 120s in this state            | `SLEEPY`        |
| `SLEEPY`            | `EMOTION` command received    | `ACTIVE`        |

- **Blink** schedule: every 3–5s in any idle state, plus on-demand via `BLINK` command.
- **Glance:** subtle x-offset shift to one side for ~400ms, returns center. Triggers every 15–25s in `IDLE_GLANCE`+.
- **Breathe:** small (±5%) width/height oscillation, period ~3s, runs continuously in `IDLE_BREATHE`+.
- **Sleepy:** shape decays into the `sleepy` emotion's slit shape; blinks become slower and longer.

### Animation

Shape-to-shape transitions use cubic-ease interpolation over **300ms**. Each `EyeShape` field is interpolated independently (width, height, all four radii, offsets, tilts).

Color transitions use a separate **400ms** crossfade between two RGB565 values, linear in RGB space.

### Render loop

```
loop():
  read_serial()           # parse any complete \n-terminated lines, mutate state
  update_state_machine()  # idle timers, blink scheduler
  update_animation()      # advance interpolation toward target
  render_frame()          # fill black, draw left eye, draw right eye
  delay(~33ms to cap at 30 FPS)
```

### Drawing eyes with tilt

`Arduino_GFX::fillRoundRect()` doesn't support rotation natively. For non-zero tilts (thinking, sad), draw into a temporary off-screen sprite buffer, then `pushImageRotated()`. Acceptable performance because tilted emotions are rare and we're not pixel-perfect rotation-perfect — 1-degree precision is fine.

---

## Pi-Side Integration

### New file: `face.py`

Thin wrapper around `pyserial`. Public API:

```python
class Face:
    def __init__(self, port: str = "/dev/ttyACM0", baud: int = 115200, timeout: float = 0.1)
    def set_emotion(self, emotion: str, led_color: str) -> None
    def blink(self) -> None
    def reset(self) -> None
    def is_connected(self) -> bool
    def close(self) -> None
```

Behavior:
- Constructor tries to open the serial port. If it fails (port doesn't exist, permission error), logs a warning and sets `_connected = False`. All subsequent calls become no-ops.
- `set_emotion(emo, color)` converts the color name (e.g., `"yellow"`) to a hex string via `config.LED_COLORS` lookup, then sends `EMOTION {emo} #{hex}\n`.
- Background thread reads incoming lines from the ESP32. On `READY`, re-sends the most recent emotion + color the Pi has on file (or a `RESET` if nothing has been sent yet this session). On `LOG`, logs at INFO. On `TOUCH`, ignores for Stage 2 (handled in 2.5).

### `brain.py` changes

- Import `Face` at the top.
- Instantiate `face = Face(config.FACE_SERIAL_PORT)` near the start of `terminal_chat()`.
- After `parse_response(raw_text)`, call `face.set_emotion(parsed["emotion"], parsed["led_color"])`.
- On `quit`, call `face.close()`.

### `config.py` additions

```python
FACE_SERIAL_PORT = "/dev/ttyACM0"   # ESP32 face controller
FACE_SERIAL_BAUD = 115200

# Convert LED_COLORS tuples to hex strings for the face protocol
def _rgb_to_hex(rgb):
    return "#{:02X}{:02X}{:02X}".format(*rgb)
LED_COLORS_HEX = {name: _rgb_to_hex(rgb) for name, rgb in LED_COLORS.items()}
```

### `requirements.txt` addition

```
pyserial>=3.5
```

---

## File Structure After Stage 2

```
Deskbot/
├── AGENTS.md
├── HARDWARE.md
├── HOUSING.md
├── README.md
├── brain.py                  # UPDATED
├── config.py                 # UPDATED
├── face.py                   # NEW
├── requirements.txt          # UPDATED
├── setup.sh
├── firmware/                 # NEW
│   └── face/                 # NEW (PlatformIO project)
│       ├── platformio.ini
│       ├── src/
│       │   ├── main.cpp
│       │   ├── face.cpp / face.h
│       │   ├── emotions.h
│       │   └── state.cpp / state.h
│       └── README.md
└── docs/
    └── superpowers/
        └── specs/
            └── 2026-05-23-stage-2-face-design.md   # this doc
```

---

## Error Handling

| Failure mode | Behavior |
|---|---|
| ESP32 unplugged on Pi side | `Face.__init__()` logs warning, all calls become no-ops, `brain.py` continues normally. |
| Bad command from Pi | ESP32 sends `LOG bad_cmd: <line>`, continues normal operation. |
| ESP32 resets mid-conversation | On reboot, ESP32 sends `READY v1.0`. Pi-side `Face` re-sends last emotion to resync. |
| Color name not in `LED_COLORS` | Pi-side defaults to `warm_white` before sending. |
| Serial port garbled bytes | ESP32 drops to the next `\n`, ignores partial line. |

---

## Testing Strategy

**Firmware bench test 1 (no Pi):** Flash a sketch that cycles all 7 emotions every 2s using only the firmware's internal state. Validates the renderer in isolation.

**Firmware bench test 2 (manual serial):** Connect Pi to ESP32, open `screen /dev/ttyACM0 115200`, type `EMOTION happy #00FF00` and similar. Visually verify each emotion + color.

**Integration test:** Run `python3 brain.py`, chat with Pip, watch the eyes change in real-time. Confirm:
- Emotion matches the speech tone
- Color matches what the AI returned
- After 5+ seconds of no input, blinks begin
- After several minutes idle, face goes sleepy
- Unplugging USB during a chat doesn't crash anything

---

## Out of Scope (Stage 2.5 / Later)

- **Capacitive touch input** — wired into the protocol (`TOUCH x,y` already reserved) but no Pi handler yet.
- **OTA firmware updates** over WiFi.
- **Sub-emotions / intensity** — `EMOTION excited:high #FFD700` to modulate animation magnitude.
- **Speech-bubble overlays** — short text snippets on screen.
- **Sound-reactive face** — eyes pulse to mic input (will integrate with Stage 3 audio).

---

## Open Questions / Risks

1. **ST7701S library availability in PlatformIO registry.** Arduino-GFX advertises ST7701 support, but Waveshare's specific panel timing may need a custom config. Plan: confirm during the very first firmware step ("draw a red square"). If Arduino-GFX can't drive this panel cleanly, fall back to Waveshare's example sketch as the base.
2. **CST816S touch chip support.** Not needed for Stage 2 but worth verifying the I²C address is reachable while we're in there.
3. **Power budget.** ESP32-S3 + 480×480 backlight + Pi 5 all on the same USB hub: marginal. May need a powered hub. To verify during integration testing.
4. **Idle timing values** (5s, 30s, 60s, 2min) are gut-feel. Easy to tune after first impressions.
