# AGENTS.md — Desk Companion Bot Project
## Read this entire file before writing any code or giving any advice.

---

## What We Are Building

An AI-powered desktop companion robot. Small, cute, friendly. It sits on a desk, listens to the user, speaks back, displays an animated face on a round screen, physically moves its head via servos, and reacts emotionally to conversations. Think a cross between a Pixar character and a smart speaker — but physical, expressive, and personal.

**The robot body is separate from the Pi housing.** The Pi 5 sits in its CanaKit case on the desk. The robot body connects to it via cables. This keeps the Pi fully reusable for other projects.

**The AI runs in the cloud via API.** The Pi handles audio I/O, serial comms to the face, servo control, and orchestration. The heavy intelligence comes from the Anthropic API over WiFi.

---

## Hardware Reality Update (Stage 2 Architecture Pivot)

The original plan called for a Pi-driven SPI display. **That is not what was built.**

**Stage 2 uses the Waveshare ESP32-S3-Touch-LCD-2.1** — an integrated dev board with:
- ESP32-S3 microcontroller
- ST7701S 480×480 round RGB LCD (parallel interface, driven directly by the ESP32)
- CST816S capacitive touch controller
- TCA9554 I²C IO expander (routes RST, CS, buzzer, etc.)

**The Pi does NOT wire to this display via GPIO or SPI.** The ESP32-S3 drives the display entirely on its own. The Pi talks to the ESP32 over a single USB-C cable. That's the entire interface between them.

See the System Architecture section below for the updated diagram.

---

## Hardware Inventory — What The User Already Has

### Brain
| Part | Notes |
|---|---|
| Raspberry Pi 5 8GB (CanaKit PRO, Turbine Black) | Main brain. Includes fan/heatsink case, 128GB SD, 45W PSU, micro-HDMI cable |
| Arduino UNO R4 WiFi | Available as secondary/helper MCU if needed |
| Arduino Nano 33 BLE Sense Rev2 | Available — has onboard IMU, mic, BLE |

### Display / Face
| Part | Notes |
|---|---|
| **Waveshare ESP32-S3-Touch-LCD-2.1** | **THE FACE — already in hand, working.** Integrated ESP32-S3 + 480×480 round LCD + touch + IO expander on one PCB. |
| Hosyond 1.28" Round TFT LCD (GC9A01, 240×240) — 3 pack | Already owned. Too small for robot face. |
| Hosyond 4.0" 480×320 TFT Touch Screen | Available but rectangular — not used. |

### Audio
| Part | Notes |
|---|---|
| AITRIP 3PCS INMP441 I²S Omnidirectional Microphone | The correct mic. Use I²S connection to Pi. |
| CQRobot Speaker 3W 8Ω | The speaker unit |
| PCM5102 Digital to Analog Audio Converter (DAC) | I²S DAC — use this to drive the speaker from Pi |

### Motion
| Part | Notes |
|---|---|
| Beffkkip 4x SG90 9g Micro Servos | 4 servos total — enough for head pan, head tilt, body wiggle, spare |
| SunFounder PCA9685 16-Channel 12-bit PWM | I²C servo driver — controls all servos from Pi with 2 wires |
| XiaoR Geek Mini Pan-Tilt Kit ($13.99) | Pre-assembled pan-tilt bracket for SG90s — this is the robot's neck/head mount |
| BOJACK L298N Motor DC Dual H-Bridge | Available for future wheel/movement upgrades |

### Lighting & Sensors
| Part | Notes |
|---|---|
| UVTaoYuan LED Strip Lights 5V USB | Mood lighting around robot body base |
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
┌─────────────────────────────────┐
│     Raspberry Pi 5 8GB          │
│  (in CanaKit case, on desk)     │
│                                 │
│  ┌──────────┐  ┌──────────────┐ │
│  │ brain.py │←→│  Anthropic   │ │
│  │          │  │  API (cloud) │ │
│  └────┬─────┘  └──────────────┘ │
│       │ face.py                 │
│       │ EMOTION happy #00FF00\n │
└───────┼─────────────────────────┘
        │ USB-C cable
        │ /dev/ttyACM0 (Pi) ↔ native USB-OTG (ESP32)
        │
┌───────┴──────────────────────────────────────┐
│  Waveshare ESP32-S3-Touch-LCD-2.1            │
│                                              │
│  ┌──────────────────────────────────────┐   │
│  │  ESP32-S3 firmware (PlatformIO/C++)  │   │
│  │  serial_parser → state machine       │   │
│  │  → face_render → ST7701S 480×480 LCD │   │
│  └──────────────────────────────────────┘   │
│  (display, touch, I²C expander all internal) │
└──────────────────────────────────────────────┘

        (Stages 3–5 — not yet built)
        │ GPIO / I²C / I²S cables
        │
┌───────┼──────────────────────────┐
│  ROBOT BODY                      │
│  ┌────┴──────────────────────┐   │
│  │ PCA9685 (I²C)             │   │
│  │  → SG90 pan servo         │   │
│  │  → SG90 tilt servo        │   │
│  │  → SG90 body wiggle       │   │
│  └───────────────────────────┘   │
│  ┌───────────────────────────┐   │
│  │ INMP441 Mic (I²S)         │   │
│  └───────────────────────────┘   │
│  ┌───────────────────────────┐   │
│  │ PCM5102 DAC + Speaker     │   │
│  └───────────────────────────┘   │
│  ┌───────────────────────────┐   │
│  │ LED Strip (5V USB/GPIO)   │   │
│  └───────────────────────────┘   │
└──────────────────────────────────┘
```

---

## Wire Protocol — Pi ↔ ESP32 (ASCII over USB serial @ 115200)

**Pi → ESP32:**
| Command | Effect |
|---|---|
| `EMOTION <name> <#RRGGBB>` | Set emotion + eye color. Name is one of: neutral, happy, excited, surprised, thinking, sad, sleepy |
| `BLINK` | Trigger a single blink |
| `RESET` | Return to neutral |
| `PING` | Connectivity check |

**ESP32 → Pi:**
| Response | Meaning |
|---|---|
| `READY v<x.y>` | Sent on boot — firmware version |
| `OK` | Command accepted |
| `PONG` | Response to PING |
| `LOG <text>` | Debug / error message |

**Pi-side:** `face.py` wraps this in a `Face` class. `face.set_emotion(name, "#RRGGBB")` is the main call. If the serial port isn't present, Face runs in silent no-op mode — brain.py doesn't need to know or care.

---

## AI Response Format — The Core Design Pattern

Every call to the Anthropic API must request this JSON format:

```json
{
  "emotion": "happy | thinking | sad | excited | neutral | surprised | sleepy",
  "speech": "What the robot says out loud",
  "movement": "nod | shake | tilt_left | tilt_right | wiggle | idle",
  "led_color": "warm_white | blue | red | purple | green | yellow"
}
```

After parsing the response, `brain.py` calls `face.set_emotion(parsed['emotion'], config.LED_COLORS_HEX[parsed['led_color']])` to drive the face. The color name → hex mapping lives in `config.py`.

---

## Build Stages

### Stage 1 — Brain Online ✅ DONE
**Goal:** Pi talks to Anthropic API, returns structured JSON, prints to terminal.
**Success criteria:** Run `python3 brain.py` and have a typed conversation that returns valid JSON responses.

Files:
- `config.py` — API keys, model settings, color maps
- `brain.py` — Core conversation loop, API calls, JSON parsing

### Stage 2 — Face 🔄 ~95% Done
**Goal:** Animated round display shows eyes that blink, change with emotion.
**Hardware:** Waveshare ESP32-S3-Touch-LCD-2.1 connected via USB-C.
**Success criteria:** `EMOTION <name> <#RRGGBB>` → face visually changes expression + animates smoothly.

Design spec: `docs/superpowers/specs/2026-05-23-stage-2-face-design.md`
Implementation plan: `docs/superpowers/plans/2026-05-23-stage-2-face.md`

**Remaining:**
- Task 17: Pi-side READY-resync (auto-recover when ESP32 is unplugged and re-plugged)
- Task 18: End-to-end integration test on Pi (brain.py + face.py together on real hardware)

Files:
- `face.py` — Pi-side serial wrapper (`Face` class)
- `firmware/face/` — Full PlatformIO project (ESP32-S3, Arduino framework)
  - `src/main.cpp` — Entry point, setup(), loop(), command dispatch
  - `src/Display_ST7701.{h,cpp}` — Custom ST7701S driver (TCA9554-routed RST/CS, ESP-IDF rgb panel)
  - `src/I2C_Driver.{h,cpp}` — Wire wrapper (SDA=15, SCL=7)
  - `src/TCA9554PWR.{h,cpp}` — IO expander driver
  - `src/emotions.h` — 7-emotion shape table (eye + eyebrow data)
  - `src/state.{h,cpp}` — FaceState, animation interpolation, blink/glance/breathe/sleepy idle
  - `src/face_render.{h,cpp}` — PSRAM framebuffer, drawing primitives
  - `src/serial_parser.{h,cpp}` — Line-based ASCII command parser
- `tests/test_face.py` — pytest suite for face.py (uses MockSerial)

**Visual design:** Hybrid Cozmo/Eve aesthetic. Two glowing rounded-rectangle eyes that morph through size, border-radius, position, and tilt. Each eye has a vertical-ellipse pupil in a lighter shade of the eye color. Each emotion has its own eyebrow shape (rounded pill above each eye). All values interpolate smoothly during transitions (300ms shape, 400ms color cubic-ease).

**Idle behaviors:**
- Blink every 6–9s (200ms close-hold-open)
- Glance every 8–15s: 3-phase (400ms rise → 2000ms hold → 400ms return) — moves pupils, not eyes
- Constant subtle pupil drift (±5px sine, ~7s period) — never perfectly still
- Breathe: ±5% size oscillation at 3s period, in IDLE_BREATHE and SLEEPY states
- Sleepy decay triggers after 215s of no commands

### Stage 3 — Voice ⬜ Pending
**Goal:** Speak to it, it speaks back.
**Hardware:** INMP441 mic (I²S), PCM5102 DAC, CQRobot speaker.
**Pipeline:** Mic audio → speech-to-text → brain.py → TTS → speaker.

Files to build:
- `audio_input.py` — Mic capture, VAD, STT
- `audio_output.py` — TTS, audio playback via DAC

### Stage 4 — Movement ⬜ Pending
**Goal:** Servos respond to movement tags.
**Hardware:** PCA9685 via I²C, 3x SG90 servos, XiaoR pan-tilt mount.

Files to build:
- `servos.py` — PCA9685 init, servo positions, movement sequences
- `choreography.py` — Named movement routines

### Stage 5 — Mood Lighting + Polish ⬜ Pending
**Goal:** LED strip color matches `led_color` tag. Wake word or touch-to-activate.

Files to build:
- `leds.py` — LED strip control
- `wake.py` — Wake word or touch detection

---

## GPIO Pin Assignments (Pi 5)

**Note: The display is no longer Pi GPIO-driven.** The ESP32-S3 handles all display, touch, and I²C IO expander connections internally. The Pi's only connection to the face hardware is a single USB-C cable.

```
USB-C (Face — ESP32-S3-Touch-LCD-2.1):
  Use the USB port on the board (native USB-OTG, NOT the UART port).
  Shows up as /dev/ttyACM0 on the Pi.

I²S Microphone (INMP441) — Stage 3:
  SCK   → GPIO 18 (Pin 12)
  WS    → GPIO 19 (Pin 35)
  SD    → GPIO 20 (Pin 38)
  L/R   → GND (left channel)

I²S DAC (PCM5102) — Stage 3:
  BCK   → GPIO 18 (Pin 12)
  LRCK  → GPIO 19 (Pin 35)
  DIN   → GPIO 21 (Pin 40)
  NOTE: Mic and DAC share I²S pins — Pi 5 supports full-duplex I²S.
  Configure as a single soundcard (resolve in Stage 3 before soldering).

I²C (PCA9685 Servo Driver) — Stage 4:
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
├── README.md              ← Quick start
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
│           └── serial_parser.h / .cpp
├── tests/
│   ├── __init__.py
│   └── test_face.py
└── docs/
    └── superpowers/
        ├── specs/2026-05-23-stage-2-face-design.md
        └── plans/2026-05-23-stage-2-face.md
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
- [x] Stage 2 firmware complete — face animates beautifully on real hardware
- [x] face.py Pi wrapper complete
- [x] tests/test_face.py — 4 tests, all passing
- [ ] Task 17: Pi-side READY-resync (ESP32 unplug/replug auto-recovery)
- [ ] Task 18: End-to-end integration test on Pi
- [ ] Stage 3: Voice (not started)
- [ ] Stage 4: Movement (not started)
- [ ] Stage 5: Polish/LEDs (not started)

---

## Important Decisions Already Made

1. **Pi 5 8GB CanaKit** is the brain — do not suggest switching the main brain
2. **Waveshare ESP32-S3-Touch-LCD-2.1** is the face — ESP32-S3 drives the display directly, Pi connects via USB only
3. **Arduino-GFX 1.4.9** is the graphics library — pinned to 1.4.9 because the `espressif32` PlatformIO platform ships Arduino-ESP32 2.0.17, and Arduino-GFX 1.5+ (with `Arduino_ST7701S_RGBPanel`) requires 3.x. Do not suggest upgrading without addressing this dependency chain.
4. **Custom display init** — standard graphics libraries (Arduino-GFX, LovyanGFX) don't drive the TCA9554 IO expander out of the box. Display init uses ESP-IDF's `esp_lcd_new_rgb_panel()` directly, adapted from https://gist.github.com/fallenartist/22d1d01e125afb02ae4ebdcdf02d1f80
5. **XiaoR Geek pan-tilt kit** is the neck mechanism — SG90 compatible, pre-assembled (Stage 4)
6. **Robot body is physically separate** from Pi housing — connected by cables
7. **Anthropic API** for intelligence — do not suggest local models for v1
8. **JSON response format** (emotion + speech + movement + led_color) is locked in
9. **Housing**: to be DIY prototyped first (PVC/containers/Dremel), then potentially 3D printed via service
10. **No arms** — head movement (pan/tilt) + body wiggle is the target motion set for v1

---

## Where To Start In A Fresh Session

1. Read this entire file
2. Check the Current Status section above
3. If continuing Stage 2: see `docs/superpowers/plans/2026-05-23-stage-2-face.md` for task list — Tasks 17 and 18 remain
4. If starting Stage 3+: ask the user which stage and review the hardware section first
5. Never skip stages — each one must work before the next begins
