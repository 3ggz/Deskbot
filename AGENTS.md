# AGENTS.md — Desk Companion Bot Project
## Read this entire file before writing any code or giving any advice.

---

## What We Are Building

An AI-powered desktop companion robot. Small, cute, friendly. It sits on a desk, listens to the user, speaks back, displays an animated face on a round screen, physically moves its head via servos, and reacts emotionally to conversations. Think a cross between a Pixar character and a smart speaker — but physical, expressive, and personal.

**The robot body is separate from the Pi housing.** The Pi 5 sits in its CanaKit case on the desk. The robot body connects to it via cables. This keeps the Pi fully reusable for other projects.

**The AI runs in the cloud via API.** The Pi handles audio I/O, display, servo control, and orchestration. The heavy intelligence comes from the Anthropic API over WiFi.

---

## Hardware Inventory — What The User Already Has

### Brain
| Part | Notes |
|---|---|
| Raspberry Pi 5 8GB (CanaKit PRO, Turbine Black) | Main brain. Includes fan/heatsink case, 128GB SD, 45W PSU, micro-HDMI cable |
| Arduino UNO R4 WiFi | Available as secondary/helper MCU if needed |
| Arduino Nano 33 BLE Sense Rev2 | Available — has onboard IMU, mic, BLE |

### Display
| Part | Notes |
|---|---|
| Hosyond 1.28" Round TFT LCD (GC9A01, 240×240) — 3 pack | Already owned. Good starter but small. |
| Hosyond 4.0" 480×320 TFT Touch Screen | Available but rectangular — not ideal for robot face |
| **Waveshare 2.1" Round LCD (480×480, GC9A01-based)** | **TARGET DISPLAY — ORDER THIS if not yet ordered. ~$20. Best face screen for this build.** |

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
| SunFounder PCA9685 16-Ch PWM | I²C servo driver (listed above) |

### Tools
| Part | Notes |
|---|---|
| Fanttik T1 Max Cordless Soldering Iron | Good quality cordless iron |
| Lesnow Solder Flux Paste + Solder Wick | Soldering consumables |
| BOJACK 840pc Solderless Breadboard Kit | Wire assortment |
| Premium Magnetic Silicone Soldering Mat | Work surface |
| Cable Matters USB-C to Micro USB cable | General use |

### Still Needed / To Order
| Part | Why | Approx Cost |
|---|---|---|
| Waveshare 2.1" Round LCD 480×480 | Upgrade from 1.28" for better face expressions | ~$20 |
| Bidirectional Logic Level Shifter | 3.3V displays/mic with Pi GPIO | ~$3 |
| Robot body shell | See HOUSING.md for options | $0–25 |

---

## System Architecture

```
┌─────────────────────────────────┐
│     Raspberry Pi 5 8GB          │
│  (in CanaKit case, on desk)     │
│                                 │
│  ┌─────────┐  ┌──────────────┐  │
│  │  Python │  │  API Client  │  │
│  │  Brain  │←→│  Anthropic   │  │
│  └────┬────┘  └──────────────┘  │
│       │                         │
└───────┼─────────────────────────┘
        │ GPIO / I²C / SPI / I²S cables
        │
┌───────┼──────────────────────────┐
│  ROBOT BODY                      │
│       │                          │
│  ┌────┴──────────────────────┐   │
│  │ PCA9685 (I²C)             │   │
│  │  → SG90 pan servo         │   │
│  │  → SG90 tilt servo        │   │
│  │  → SG90 body wiggle       │   │
│  └───────────────────────────┘   │
│  ┌───────────────────────────┐   │
│  │ Round Display (SPI)       │   │
│  │  Waveshare 2.1" 480×480   │   │
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

## AI Response Format — The Core Design Pattern

Every call to the Anthropic API must request this JSON format. This single structure drives the face, voice, and movement simultaneously:

```json
{
  "emotion": "happy | thinking | sad | excited | neutral | surprised | sleepy",
  "speech": "What the robot says out loud",
  "movement": "nod | shake | tilt_left | tilt_right | wiggle | idle",
  "led_color": "warm_white | blue | red | purple | green | yellow"
}
```

The system prompt to send with every conversation:

```
You are a small, friendly desktop robot companion named [NAME TBD by user].
You are witty, warm, curious, and occasionally playful. You live on the user's desk.
You have a physical body with a round screen face, and you can move your head.

Always respond ONLY as valid JSON with exactly these fields:
{
  "emotion": one of: happy, thinking, sad, excited, neutral, surprised, sleepy,
  "speech": your spoken response (keep under 2 sentences for natural pacing),
  "movement": one of: nod, shake, tilt_left, tilt_right, wiggle, idle,
  "led_color": one of: warm_white, blue, red, purple, green, yellow
}

Emotion guide:
- thinking: use while processing complex questions
- excited: good news, interesting topics, greetings
- happy: general positive responses
- sad: bad news, apologies
- surprised: unexpected information
- sleepy: idle state, late night
- neutral: factual responses

Never break character. Never output anything outside the JSON object.
```

---

## Build Stages

### Stage 1 — Brain Online ← CURRENT STAGE
**Goal:** Pi talks to Anthropic API, returns structured JSON, prints to terminal.
**Success criteria:** Run `python3 brain.py` and have a typed conversation that returns valid JSON responses.
**No hardware needed** beyond the Pi itself.

Files to build:
- `src/config.py` — API keys, model settings
- `src/brain.py` — Core conversation loop, API calls, JSON parsing
- `src/test_brain.py` — Unit tests for the brain module

### Stage 2 — Face
**Goal:** Animated round display shows eyes that blink, change with emotion.
**Hardware:** Waveshare 2.1" round display wired via SPI.
**Success criteria:** Emotion variable changes → face visually changes expression.

Files to build:
- `src/display.py` — Face rendering, animation loop
- `assets/emotions/` — Emotion face definitions (eye shapes, colors)

### Stage 3 — Voice
**Goal:** Speak to it, it speaks back.
**Hardware:** INMP441 mic (I²S), PCM5102 DAC, CQRobot speaker.
**Pipeline:** Mic audio → speech-to-text (Whisper API or Google STT) → brain.py → TTS → speaker.
**Success criteria:** Full spoken conversation end-to-end.

Files to build:
- `src/audio_input.py` — Mic capture, VAD (voice activity detection), STT
- `src/audio_output.py` — TTS, audio playback via DAC

### Stage 4 — Movement
**Goal:** Servos respond to emotion and movement tags.
**Hardware:** PCA9685 via I²C, 3x SG90 servos, XiaoR pan-tilt mount.
**Success criteria:** "wiggle" tag → robot wiggles. "nod" tag → robot nods.

Files to build:
- `src/servos.py` — PCA9685 init, servo positions, movement sequences
- `src/choreography.py` — Named movement routines (dance, idle, react)

### Stage 5 — Mood Lighting + Polish
**Goal:** LED strip color matches `led_color` tag. Wake word or touch-to-activate.
**Hardware:** LED strip on GPIO PWM or via relay, touch sensor from 37-in-1 kit.

Files to build:
- `src/leds.py` — LED strip control
- `src/wake.py` — Wake word or touch detection

### Stage 6 (Future) — Vision
**Goal:** Pi camera or USB webcam enables face tracking, presence detection.
**Note:** The XiaoR pan-tilt webcam (not included with the bracket) plugs in here.

---

## GPIO Pin Assignments (Pi 5)

```
SPI (Display):
  MOSI  → GPIO 10 (Pin 19)
  MISO  → GPIO 9  (Pin 21)
  SCLK  → GPIO 11 (Pin 23)
  CS    → GPIO 8  (Pin 24)
  DC    → GPIO 25 (Pin 22)
  RST   → GPIO 27 (Pin 13)
  BL    → GPIO 18 (Pin 12) [backlight PWM]

I²S Microphone (INMP441):
  SCK   → GPIO 18 (Pin 12) — NOTE: conflicts with BL if used simultaneously, resolve in wiring
  WS    → GPIO 19 (Pin 35)
  SD    → GPIO 20 (Pin 38)
  L/R   → GND (left channel)

I²S DAC (PCM5102):
  BCK   → GPIO 18 (Pin 12)
  LRCK  → GPIO 19 (Pin 35)
  DIN   → GPIO 21 (Pin 40)

I²C (PCA9685 Servo Driver):
  SDA   → GPIO 2  (Pin 3)
  SCL   → GPIO 3  (Pin 5)

NOTE: GPIO 18/19 conflict between mic and DAC is resolved by using
separate I²S buses or time-multiplexing (only one active at a time).
Resolve this properly in Stage 3 before soldering anything.
```

---

## Code Style & Conventions

- Language: **Python 3.11+**
- Style: Clean, well-commented, modular. Each hardware system gets its own file.
- Error handling: All hardware interactions wrapped in try/except with graceful fallback
- Logging: Use Python `logging` module, not print statements (except during dev)
- Config: All secrets and pin numbers in `src/config.py`, never hardcoded
- No blocking calls on the main thread — use asyncio or threading for audio/display

---

## Project File Structure

```
desk-companion-bot/
├── AGENTS.md              ← You are here. Read first.
├── HARDWARE.md            ← Wiring diagrams and pin reference
├── HOUSING.md             ← Robot body build options
├── README.md              ← Quick start
├── requirements.txt       ← Python dependencies
├── src/
│   ├── config.py          ← API keys, GPIO pins, settings (DO NOT COMMIT KEYS)
│   ├── brain.py           ← Core AI conversation loop
│   ├── display.py         ← Face rendering and animation
│   ├── audio_input.py     ← Mic capture and speech-to-text
│   ├── audio_output.py    ← Text-to-speech and playback
│   ├── servos.py          ← PCA9685 and servo control
│   ├── choreography.py    ← Named movement sequences
│   ├── leds.py            ← LED strip control
│   ├── wake.py            ← Wake word / touch detection
│   └── main.py            ← Entry point — orchestrates all modules
├── assets/
│   └── emotions/          ← Face expression definitions
├── tests/
│   ├── test_brain.py
│   ├── test_display.py
│   └── test_servos.py
└── scripts/
    ├── setup.sh           ← Installs all dependencies on Pi
    └── test_hardware.py   ← Interactive hardware validation tool
```

---

## Current Status

- [x] All hardware ordered and arriving
- [x] CanaKit Pi 5 8GB in hand — **START HERE**
- [ ] Pi OS flashed and booted
- [ ] WiFi connected
- [ ] Anthropic API key configured
- [ ] Stage 1 complete
- [ ] Remaining parts arriving (displays, servos, mic, speaker, etc.)

---

## Important Decisions Already Made

1. **Pi 5 8GB CanaKit** is the brain — do not suggest switching to Arduino/ESP32 as main brain
2. **Waveshare 2.1" round display** is the target face screen (upgrading from 1.28" GC9A01)
3. **XiaoR Geek pan-tilt kit** is the neck mechanism — SG90 compatible, pre-assembled
4. **Robot body is physically separate** from Pi housing — connected by cables
5. **Anthropic API** for intelligence — do not suggest local models for v1
6. **JSON response format** (emotion + speech + movement + led_color) is locked in
7. **Housing**: to be DIY prototyped first (PVC/containers/Dremel), then potentially 3D printed via service
8. **No arms** — head movement (pan/tilt) + body wiggle is the target motion set for v1

---

## Where To Start In A Fresh Session

1. Read this entire file
2. Check `src/config.py` for current state
3. Ask the user: "Which stage are we working on?" if unclear
4. If Stage 1: help write/run `src/brain.py` — get API talking first
5. Never skip stages — each one must work before the next begins
