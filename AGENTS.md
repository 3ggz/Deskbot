# AGENTS.md — Desk Companion Bot Project
## Read this entire file before writing any code or giving any advice.

---

## What We Are Building

An AI-powered desktop companion robot. Small, cute, friendly. It sits on a desk, listens to the user, speaks back, displays an animated face on a round screen, physically moves its head via servos, and reacts emotionally to conversations. Think a cross between a Pixar character and a smart speaker — but physical, expressive, and personal.

The robot goes by **Pip**.

**The robot body is separate from the Pi housing.** The Pi 5 sits in its CanaKit case on the desk. The robot body connects to it via cables. This keeps the Pi fully reusable for other projects.

**The AI runs in the cloud via API.** The Pi handles audio I/O, serial comms to the face, servo control, and orchestration. The heavy intelligence comes from the Anthropic API over WiFi.

---

## Hardware Reality Update (Stage 2 Architecture Pivot)

The original plan called for a Pi-driven SPI display. **That is not what was built.**

**Stage 2 uses the Waveshare ESP32-S3-Touch-LCD-2.1** — an integrated dev board with:
- ESP32-S3 microcontroller
- ST7701S 480×480 round RGB LCD (parallel interface, driven directly by the ESP32)
- CST816S capacitive touch controller (I²C internal)
- TCA9554 I²C IO expander (routes RST, CS, buzzer, etc.)

**The Pi does NOT wire to this display via GPIO or SPI.** The ESP32-S3 drives the display entirely on its own. The Pi talks to the ESP32 over a single USB-C cable. That's the entire interface between them.

**Servos are also driven from the ESP32**, not from the Pi. Pan on GPIO43, tilt on GPIO44. This was delivered early as part of Stage 2 polish and is fully working.

---

## Hardware Inventory — What The User Already Has

### Brain
| Part | Notes |
|---|---|
| Raspberry Pi 5 8GB (CanaKit PRO, Turbine Black) | Main brain. Includes fan/heatsink case, 128GB SD, 45W PSU, micro-HDMI cable |
| Arduino UNO R4 WiFi | Available as secondary/helper MCU if needed |
| Arduino Nano 33 BLE Sense Rev2 | Planned Stage 3b: built-in PDM mic + IMU + BLE → handheld mic remote |

### Display / Face
| Part | Notes |
|---|---|
| **Waveshare ESP32-S3-Touch-LCD-2.1** | **THE FACE — working.** Integrated ESP32-S3 + 480×480 round LCD + CST816 touch + TCA9554 IO expander. |
| Hosyond 1.28" Round TFT LCD (GC9A01, 240×240) — 3 pack | Owned. Too small for robot face. Parked. |
| Hosyond 4.0" 480×320 TFT Touch Screen | Available but rectangular — not used. |

### Audio
| Part | Notes |
|---|---|
| AITRIP 3PCS INMP441 I²S Omnidirectional Microphone | The correct mic. Use I²S connection to Pi (or Nano 33). |
| CQRobot Speaker 3W 8Ω | The speaker unit |
| PCM5102 Digital to Analog Audio Converter (DAC) | I²S DAC — wired and tested, **but doesn't drive speaker loud enough without an amp**. Paused. |

### Motion
| Part | Notes |
|---|---|
| Beffkkip 4x SG90 9g Micro Servos | 4 servos total — 2 in use (ESP32 GPIO43/44), 2 spare |
| SunFounder PCA9685 16-Channel 12-bit PWM | Parked — future Stage 4.5 for more axes |
| XiaoR Geek Mini Pan-Tilt Kit ($13.99) | Pre-assembled pan-tilt bracket — the robot's neck/head mount |
| BOJACK L298N Motor DC Dual H-Bridge | Available for future wheel/movement upgrades |

### Lighting & Sensors
| Part | Notes |
|---|---|
| UVTaoYuan LED Strip Lights 5V USB | Mood lighting around robot body base (Stage 5) |
| ELEGOO 37-in-1 Sensor Module Kit | Includes touch, IR, sound, light sensors — use as needed |
| Gikfun MTS102 Mini Toggle Switches | Physical switches — use for power or mode selection |
| ALLECIN WH148 Potentiometer Kit | Volume/sensitivity knobs if desired |

### Prototyping & Power
| Part | Notes |
|---|---|
| ELEGOO 2PCS 830pt + 2PCS 400pt Breadboards | Prototyping |
| ELEGOO 120pcs Dupont Jumper Wires | All connection types |
| Arduino Breadboard Power Supply (3.3V/5V) | Bench power for breadboard work |
| SunFounder BreadVolt Breadboard Power Supply | Additional bench power |

### Tools
| Part | Notes |
|---|---|
| Fanttik T1 Max Cordless Soldering Iron | Good quality cordless iron |
| Lesnow Solder Flux Paste + Solder Wick | Soldering consumables |
| BOJACK 840pc Solderless Breadboard Kit | Wire assortment |
| Premium Magnetic Silicone Soldering Mat | Work surface |

---

## System Architecture

```
┌─────────────────────────────────────────────────┐
│     Raspberry Pi 5 8GB                           │
│  (in CanaKit case, on desk)                      │
│                                                  │
│  ┌──────────┐  ┌──────────────┐                  │
│  │ brain.py │←→│  Anthropic   │                  │
│  │          │  │  API (cloud) │                  │
│  └────┬─────┘  └──────────────┘                  │
│       │ face.py                                  │
│       │ EMOTION happy #00FF00\n                  │
│       │ TEXT_LIVE Hello...\n                     │
│       │ SERVO nod\n                              │
└───────┼──────────────────────────────────────────┘
        │ USB-C cable
        │ /dev/ttyACM0 (Pi) ↔ native USB-OTG (ESP32)
        │
┌───────┴─────────────────────────────────────────────┐
│  Waveshare ESP32-S3-Touch-LCD-2.1                   │
│                                                     │
│  ┌─────────────────────────────────────────────┐   │
│  │  ESP32-S3 firmware (PlatformIO/C++)          │   │
│  │  serial_parser → state machine               │   │
│  │    → face_render → ST7701S 480×480 LCD       │   │
│  │    → servos (GPIO43 pan, GPIO44 tilt)        │   │
│  │    → touch polling (CST816, 25Hz)            │   │
│  │    → captions (word-wrap + pagination)       │   │
│  └─────────────────────────────────────────────┘   │
│  (display, touch, I²C expander, servos — internal) │
└─────────────────────────────────────────────────────┘

        (Stages 3–5 — not yet built)
        │ GPIO / I²C / I²S cables
        │
┌───────┼──────────────────────────┐
│  ROBOT BODY                      │
│  ┌────────────────────────────┐  │
│  │ PCA9685 (I²C) — future     │  │
│  │  → additional SG90 servos  │  │
│  └────────────────────────────┘  │
│  ┌────────────────────────────┐  │
│  │ INMP441 Mic (I²S) — S3b   │  │
│  └────────────────────────────┘  │
│  ┌────────────────────────────┐  │
│  │ PCM5102 DAC + amp + Speaker│  │
│  │ (paused — needs amp)       │  │
│  └────────────────────────────┘  │
│  ┌────────────────────────────┐  │
│  │ LED Strip (5V USB/GPIO)    │  │
│  └────────────────────────────┘  │
└──────────────────────────────────┘

        (Stage 3b — planned)
        │ BLE / USB
        │
┌───────┴──────────────────────────────┐
│  Arduino Nano 33 BLE Sense Rev2      │
│  (handheld mic remote concept)       │
│  Built-in PDM mic, IMU, BLE/USB      │
└──────────────────────────────────────┘
```

---

## Pip's Face — What It Actually Looks Like

Pip's face is two glowing rounded-rectangle eyes on a black background. Each eye has:
- A vertical-ellipse **pupil** in a lighter shade of the eye color, drawn inside the eye
- An **eyebrow** — a rounded pill above the eye, independently tuned per emotion
- All dimensions, radii, positions, and tilts **interpolate smoothly** (300ms shape, 400ms color, cubic-ease)

Pupils auto-hide when eye height < 30px (sleepy slits, blink, wink) — prevents the dead-fish stare.

### 12 Emotions

| Emotion | Eyes | Brows | Character |
|---|---|---|---|
| neutral | Tall rounded bars | Flat, level | Calm, alert |
| happy | Wide arches (flat bottom, curved top) | Lifted, slight outer tilt | Warm smile |
| excited | Taller than neutral, fully round | Higher with slight outward tilt | Big energy |
| surprised | Circular, wider | Shot way up, wider | Oh! |
| thinking | Asymmetric: left tilted down, right tilted up | Clearly asymmetric | Pondering |
| sad | Inverted arches (flat top, curved bottom) | Inner ends raised (upside-down V) | Soft frown |
| sleepy | Collapsed slits | Low, thin | Half asleep |
| angry | Narrowed and slightly shifted down | Inner ends angled DOWN (opposite of sad) | Irritated |
| love | Wide, bright, rounder than surprised | Gently lifted with slight outward tilt | Adoring |
| confused | One eye taller than the other; slight tilt | Dramatically asymmetric (one high, one normal) | Huh? |
| embarrassed | Slightly smaller, shifted down | Lifted "innocently" | Sheepish |
| wink | Left eye collapsed to slit, right eye happy | Both retain happy brow | Playful |

---

## Idle Behavior State Machine

After some period of no commands, Pip gradually winds down:

```
Command arrives → ACTIVE (reset timer)
       ↓ 5s no commands
  IDLE_BLINK    — blinks every 6–9s
       ↓ 35s
  IDLE_GLANCE   — blinks + glances every 8–15s
                  (3-phase: 400ms rise → 2000ms hold → 400ms return)
       ↓ 95s
  IDLE_BREATHE  — blinks + glances + ±5% eye size oscillation (3s period)
       ↓ 215s
    SLEEPY       — all of the above, slits, slower breathing

Always: ±5px pupil drift (sine, ~7s period) — never perfectly still.
```

**Auto-return to baseline:** 4s after an EMOTION command, Pip's *shape* eases back to neutral. The color stays — that's the persistent mood.

---

## The Dream System

After ~60s of inactivity, a background thread on the Pi wakes up every 25–50s (random interval) and calls the Anthropic API with a dreaming prompt. The response is sent as a TEXT command and appears as a caption on Pip's face. 30% of the time it includes the current time (so Pip can mutter something like "...2:47 already..."). This only runs on Linux (the live-typing feature uses `termios` cbreak mode, which guards the thread).

---

## Servo System

Two SG90 servos are wired directly to the ESP32-S3:
- **Pan:** GPIO43 (left/right)
- **Tilt:** GPIO44 (up/down)
- **Power:** 5V from the board's power rail, common GND

The ESP32Servo library (MCPWM peripheral) drives them — LEDC was evaluated and found unreliable for servos on ESP32-S3.

**Default limits:** pan 30–150°, tilt 60–120°, center 90°

**Float-precision interpolation** with cubic ease. No staircase motion at slow speeds.

**Ambient drift:** After 30s of no servo commands, the head drifts subtly ±5° pan / ±4° tilt every 8–15s.

**Script cancellation:** A direct PAN or TILT command immediately cancels any running script.

### 9 Named Movements (+ 1 hidden)

| Movement | Description |
|---|---|
| nod | Tilt down → up → center (yes) |
| shake | Pan left → right → center (no) |
| tilt_left | Small lean left, hold, return |
| tilt_right | Small lean right, hold, return |
| wiggle | Alternating pan oscillations |
| idle | Return to center |
| look_around | Slow scan of all four corners |
| bow | Deep tilt-down, hold, return |
| dance | 24-step big-energy dance with circular flourish |
| dance_crazy | 36-step ~8s showstopper — hidden, not exposed to brain.py (manual SERVO command only) |

All named movements use both axes for natural multi-axis motion.

---

## Touch / Poke System

The CST816 capacitive touch controller is polled at 25Hz via I²C. On a touch rising edge (finger down, first contact only):

- 1 of 6 random flinch variants fires: emotion + caption + servo recoil
- Variants: "Hey!" (surprised), "Oof!" (surprised), "Eek!" (surprised), "What?!" (confused), "Quit it!" (angry), "Hehe!" (happy)
- Each pairs a different color and a different servo flinch direction (back/left/right)
- **1.5s cooldown** prevents repeat-fire from holding a finger down
- Touch coordinates are sent to the Pi as `LOG TOUCH x,y`

---

## Caption System

Captions appear in a translucent 2-line box at y=295 on the round 480×480 LCD.

- **Word-wrap:** 40 chars/line, up to 12 lines stored
- **Pagination:** Long text pages through at 3.5s per page automatically
- **TEXT** holds for 4.5s (longer for paginated text)
- **TEXT_LIVE** holds for 60s — designed for live-typing: every keystroke on the Pi appears on Pip's face in real time (Linux only — uses `termios` cbreak mode in brain.py)
- **TEXT_CLEAR** dismisses immediately

---

## Wire Protocol — Pi ↔ ESP32 (ASCII over USB serial @ 115200)

### Pi → ESP32

| Command | Effect |
|---|---|
| `EMOTION <name> <#RRGGBB>` | Set emotion + eye color. Auto-returns to neutral shape after 4.5s; color persists. |
| `BLINK` | Trigger a single blink immediately |
| `RESET` | Return to neutral shape and cyan color, recenter servos |
| `PING` | Heartbeat — expects `PONG` |
| `TEXT <message>` | Show caption, 4.5s default (longer for paginated) |
| `TEXT_LIVE <message>` | Show caption, hold 60s (live-typing mode) |
| `TEXT_CLEAR` | Dismiss caption immediately |
| `SERVO <action>` | Run a named movement: nod / shake / tilt_left / tilt_right / wiggle / idle / dance / look_around / bow (+ dance_crazy hidden) |
| `PAN <deg>` | Move pan servo to absolute angle, cancel any active script |
| `TILT <deg>` | Move tilt servo to absolute angle, cancel any active script |
| `SERVO_LIMITS <pmin> <pmax> <tmin> <tmax>` | Live-tune travel limits |
| `SERVO_TRIM <ptrim> <ttrim>` | Live-tune center trim |
| `SERVO_INFO` | Print current servo state to serial |
| `SERVO_TEST_HIGH` / `SERVO_TEST_LOW` / `SERVO_RESET` | Diagnostic raw GPIO toggle / reinit |

Valid emotion names: `neutral` `happy` `excited` `surprised` `thinking` `sad` `sleepy` `angry` `love` `confused` `embarrassed` `wink`

### ESP32 → Pi

| Response | Meaning |
|---|---|
| `READY v2.4` | Sent on boot — firmware version |
| `OK` | Command accepted |
| `PONG` | Response to PING |
| `LOG <text>` | Debug / diagnostic message |
| `LOG TOUCH <x>,<y>` | Touch event coordinates |

**Pi-side:** `face.py` wraps this in a `Face` class. `face.set_emotion(name, "#RRGGBB")` and `face.move(action)` are the main calls. If the serial port isn't present, Face runs in silent no-op mode — brain.py doesn't need to know or care.

---

## AI Response Format — The Core Design Pattern

Every call to the Anthropic API must request this JSON format:

```json
{
  "emotion": "neutral | happy | excited | surprised | thinking | sad | sleepy | angry | love | confused | embarrassed | wink",
  "speech": "What the robot says out loud",
  "movement": "nod | shake | tilt_left | tilt_right | wiggle | idle | dance | look_around | bow",
  "led_color": "warm_white | blue | red | purple | green | yellow"
}
```

After parsing the response, `brain.py` calls `face.set_emotion(...)` and `face.move(...)`. The color name → hex mapping lives in `config.py`. `dance_crazy` is intentionally not exposed here — it's a manual Easter egg only.

---

## Build Stages

### Stage 1 — Brain Online ✅ DONE
**Goal:** Pi talks to Anthropic API, returns structured JSON, prints to terminal.

Files: `config.py`, `brain.py`

### Stage 2 — Face ✅ DONE++ (bonus features included)
**Goal:** Animated round display, emotions, idle behaviors, captions, touch, servos.
**Hardware:** Waveshare ESP32-S3-Touch-LCD-2.1 connected via USB-C.

What got built: 12 emotions, eyebrows, pupils, idle state machine, dreams, live captions, touch/poke, 2-servo pan-tilt, 9 movement scripts + dance_crazy, ambient drift.

Files:
- `face.py` — Pi-side serial wrapper (`Face` class)
- `firmware/face/` — Full PlatformIO project
  - `src/main.cpp` — Entry point, setup(), loop(), command dispatch
  - `src/Display_ST7701.{h,cpp}` — Custom ST7701S driver (TCA9554-routed RST/CS, ESP-IDF rgb panel)
  - `src/I2C_Driver.{h,cpp}` — Wire wrapper (SDA=15, SCL=7)
  - `src/TCA9554PWR.{h,cpp}` — IO expander driver
  - `src/emotions.h` — 12-emotion shape + eyebrow table
  - `src/state.{h,cpp}` — FaceState, animation interpolation, idle state machine, captions, dreams
  - `src/face_render.{h,cpp}` — PSRAM framebuffer, drawing primitives, caption rendering
  - `src/serial_parser.{h,cpp}` — Line-based ASCII command parser
  - `src/servos.{h,cpp}` — Two-servo pan-tilt, smooth interpolation, named movements
  - `src/touch.{h,cpp}` — CST816 capacitive touch polling
- `tests/test_face.py` — pytest suite for face.py (uses MockSerial)

**Remaining:**
- Task 17: Pi-side READY-resync (auto-recover when ESP32 is unplugged and re-plugged mid-session)
- Task 18: End-to-end integration test on Pi (brain.py + face.py together on real hardware)

### Stage 3a — Audio Out ⏸ PAUSED (hardware gap)
**Goal:** Pip speaks back via TTS → speaker.
**Hardware:** PCM5102 DAC wired and tested. Problem: it doesn't drive the CQRobot passive speaker loud enough without an amplifier.
**Next step:** Acquire MAX98357A I²S amplifier (~$5) or a powered USB speaker, then resume.

Files to build: `audio_output.py`

### Stage 3b — Audio In ⬜ PLANNED
**Goal:** Clean voice capture close to the user.
**Hardware plan:** Arduino Nano 33 BLE Sense Rev2 as a handheld mic remote. Built-in PDM mic + IMU + BLE. Can be brought close to the user for clear audio, away from desk noise.

Files to build: `audio_input.py` (and possibly Nano 33 firmware sketch)

### Stage 4 — Movement (Extended) ⬜ Pending / Partial
**Current:** 2-servo pan-tilt already done (ESP32 GPIO43/44).
**Remaining (Stage 4.5):** Add PCA9685 via I²C for body wiggle, additional axes.
**Hardware:** SunFounder PCA9685 (owned, parked), XiaoR pan-tilt kit (the neck mount).

Files to build: `servos.py` (Pi-side PCA9685 control, when/if needed)

### Stage 5 — Mood Lighting + Polish ⬜ Pending
**Goal:** LED strip color matches `led_color` tag. Wake word or touch-to-activate.

Files to build: `leds.py`, `wake.py`

---

## GPIO Pin Assignments (Pi 5)

**Note: The display and servos are no longer Pi GPIO-driven.** The ESP32-S3 handles everything: display, touch, I²C IO expander, and servos. The Pi's only connection to the face hardware is a single USB-C cable.

```
USB-C (Face — ESP32-S3-Touch-LCD-2.1):
  Use the USB port on the board (native USB-OTG, NOT the UART port).
  Shows up as /dev/ttyACM0 on the Pi.

I²S Microphone (INMP441) — Stage 3b:
  SCK   → GPIO 18 (Pin 12)
  WS    → GPIO 19 (Pin 35)
  SD    → GPIO 20 (Pin 38)
  L/R   → GND (left channel)

I²S DAC (PCM5102) — Stage 3a (paused):
  BCK   → GPIO 18 (Pin 12)
  LRCK  → GPIO 19 (Pin 35)
  DIN   → GPIO 21 (Pin 40)
  NOTE: Mic and DAC share I²S pins — Pi 5 supports full-duplex I²S.
  Configure as a single soundcard (resolve in Stage 3 before soldering).

I²C (PCA9685 Servo Driver) — Stage 4.5:
  SDA   → GPIO 2  (Pin 3)
  SCL   → GPIO 3  (Pin 5)
```

---

## Project File Structure

```
Deskbot/
├── AGENTS.md              ← You are here. Read first.
├── HARDWARE.md            ← Wiring diagrams and component reference
├── HOUSING.md             ← Robot body build options
├── README.md              ← Quick start + current features
├── brain.py               ← Core AI conversation loop
├── config.py              ← API keys, GPIO pins, color maps (DO NOT COMMIT KEYS)
├── face.py                ← Pi-side serial wrapper for ESP32 face
├── requirements.txt       ← Python dependencies
├── setup.sh               ← One-shot Pi setup script
├── .gitignore
├── .gitattributes
├── firmware/
│   └── face/              ← PlatformIO project (ESP32-S3 face firmware)
│       ├── platformio.ini
│       ├── README.md
│       └── src/
│           ├── main.cpp
│           ├── Display_ST7701.h / .cpp
│           ├── I2C_Driver.h / .cpp
│           ├── TCA9554PWR.h / .cpp
│           ├── emotions.h
│           ├── face_render.h / .cpp
│           ├── state.h / .cpp
│           ├── serial_parser.h / .cpp
│           ├── servos.h / .cpp
│           └── touch.h / .cpp
├── tests/
│   ├── __init__.py
│   └── test_face.py
└── docs/
    └── superpowers/
        ├── specs/2026-05-23-stage-2-face-design.md   ← historical, do not edit
        └── plans/2026-05-23-stage-2-face.md           ← historical, do not edit
```

---

## Code Style & Conventions

- **Pi side:** Python 3.11+. Clean, well-commented, modular. Each hardware system gets its own file.
- **Firmware side:** Arduino/C++ via PlatformIO. Each subsystem gets its own .h/.cpp pair.
- Error handling: All hardware interactions wrapped in try/except (Python) or null-check (C++) with graceful fallback
- Logging: Python uses `logging` module, not print(). Firmware uses `Serial.println("LOG ...")`.
- Config: All secrets and pin numbers in `config.py`, never hardcoded
- No blocking calls on the main thread — use asyncio or threading for audio/display (Pi side)

---

## Current Status

- [x] All hardware ordered and in hand
- [x] Pi OS flashed, booted, WiFi connected
- [x] Anthropic API key configured
- [x] Stage 1 complete — brain.py works, returns structured emotion JSON
- [x] Stage 2 firmware complete — face animates beautifully on real hardware (READY v2.4)
- [x] 12 emotions with eyebrows and pupils
- [x] Idle state machine (active → blink → glance → breathe → sleepy)
- [x] Dream system (Pi background thread → API → caption)
- [x] Multi-line captions with word-wrap, pagination, live-typing
- [x] Touch/poke system with 6 random flinch variants
- [x] Two-servo pan-tilt (ESP32 GPIO43/44) with 9 movement scripts
- [x] Ambient servo drift
- [x] face.py Pi wrapper complete
- [x] tests/test_face.py — passing
- [ ] Task 17: Pi-side READY-resync (ESP32 unplug/replug auto-recovery)
- [ ] Task 18: End-to-end integration test on Pi
- [ ] Stage 3a: Audio out (paused — needs amplifier hardware)
- [ ] Stage 3b: Audio in (planned — Nano 33 BLE Sense remote)
- [ ] Stage 4.5: PCA9685 for additional servo axes
- [ ] Stage 5: LEDs + polish

---

## Wishlist / Next Steps

1. **Audio hardware:** MAX98357A I²S amplifier or powered USB speaker — unblocks Stage 3a TTS
2. **Nano 33 BLE Sense remote:** Stage 3b mic/IMU input remote concept
3. **Eyes-as-digits time display:** Idle mode where Pip's eyes morph to show the current time (digits from eye shapes)
4. **More emotes:** Expand the movement library — "thinking" head tilt held for long pauses, "listening" lean-in, etc.
5. **Pi-side READY-resync (Task 17):** Auto-recover serial state when ESP32 is unplugged mid-session
6. **Housing:** Prototype with PVC/container, then 3D print for final version

---

## Important Decisions Already Made

1. **Pi 5 8GB CanaKit** is the brain — do not suggest switching the main brain
2. **Waveshare ESP32-S3-Touch-LCD-2.1** is the face — ESP32-S3 drives the display directly, Pi connects via USB only
3. **Servos on ESP32, not Pi** — GPIO43 (pan) and GPIO44 (tilt) driven from the ESP32-S3 via ESP32Servo (MCPWM). Do not suggest moving servos to Pi GPIO or PCA9685 for the current 2-servo setup.
4. **Arduino-GFX 1.4.9** is the graphics library — pinned to 1.4.9 because the `espressif32` PlatformIO platform ships Arduino-ESP32 2.0.17, and Arduino-GFX 1.5+ (with `Arduino_ST7701S_RGBPanel`) requires 3.x. Do not suggest upgrading without addressing this dependency chain.
5. **Custom display init** — standard graphics libraries (Arduino-GFX, LovyanGFX) don't drive the TCA9554 IO expander out of the box. Display init uses ESP-IDF's `esp_lcd_new_rgb_panel()` directly, adapted from https://gist.github.com/fallenartist/22d1d01e125afb02ae4ebdcdf02d1f80
6. **XiaoR Geek pan-tilt kit** is the neck mechanism — SG90 compatible, pre-assembled (future Stage 4.5 for additional axes)
7. **Robot body is physically separate** from Pi housing — connected by cables
8. **Anthropic API** for intelligence — do not suggest local models for v1
9. **JSON response format** (emotion + speech + movement + led_color) is locked in
10. **Housing**: to be DIY prototyped first (PVC/containers/Dremel), then potentially 3D printed via service
11. **No arms** — head movement (pan/tilt) + body wiggle is the target motion set for v1
12. **PCM5102 without amp is insufficient** — do not suggest wiring it to a passive speaker without an amplifier

---

## Where To Start In A Fresh Session

1. Read this entire file
2. Check the Current Status section above
3. If continuing Stage 2 cleanup: Tasks 17 and 18 remain
4. If starting Stage 3a: confirm amplifier hardware is in hand first; do not proceed on PCM5102 alone
5. If starting Stage 3b: review the Nano 33 BLE Sense Rev2 capabilities and the handheld remote concept
6. Never skip stages — each one must work before the next begins
