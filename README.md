# Desk Companion Bot

A small, expressive AI-powered robot that sits on your desk. Animated round face, physical head movement, voice conversation, mood lighting.

**Read `AGENTS.md` first** — it contains the full project context, hardware inventory, architecture decisions, and build stages.

---

## Quick Start

### Stage 1 — Brain only (Pi)

```bash
# 1. Clone / copy this folder to your Pi
# 2. Run setup
bash setup.sh

# 3. Add your API key
nano .env  # Set ANTHROPIC_API_KEY

# 4. Run
source venv/bin/activate
python3 brain.py
```

If the bot talks back in the terminal with emotion tags — Stage 1 is done. ✅

### Stage 2 — Face (ESP32-S3 + Pi together)

**On Windows (build machine):**
```bash
# 1. Install PlatformIO (VS Code extension or CLI)
cd firmware/face
pio run -t upload       # compile and flash ESP32 via USB
pio device monitor      # confirm "READY v2.4" on boot
```

**On Pi (runtime):**
```bash
source venv/bin/activate
python3 brain.py        # Face auto-connects at /dev/ttyACM0
```

Plug the ESP32 into the Pi via USB-C. The face connects automatically — brain.py runs fine without it if it's not plugged in.

---

## Current Features

Pip is fully expressive today. This is what works on real hardware:

**Face / emotions**
- 12 emotions: neutral, happy, excited, surprised, thinking, sad, sleepy, angry, love, confused, embarrassed, wink
- Per-eye eyebrows, vertical-ellipse pupils that scale with eye height
- Pupils auto-hide when eyes are nearly closed (blink, wink, sleepy) — no dead-fish stare
- Smooth cubic-ease interpolation between shapes (300ms) and colors (400ms)
- 4s auto-return to neutral shape after an emotion arrives (color stays as the persistent mood)

**Idle behavior**
- Active → idle blink (5s) → idle glance (35s) → idle breathe (95s) → sleepy (215s)
- Blinks every 6–9s; 3-phase glances every 8–15s; constant ±5px pupil drift; ±5% breathing oscillation
- After 60s idle: background thread on the Pi calls the Anthropic API every 25–50s to generate dreamy mumbles shown as captions ("Pip dreams")

**Servos (head pan + tilt)**
- Two SG90 servos driven directly from the ESP32-S3 (GPIO43 pan, GPIO44 tilt)
- Float-precision cubic-ease interpolation — no staircase stutter
- 9 named movements: nod, shake, tilt_left, tilt_right, wiggle, idle, dance, look_around, bow
- After 30s idle, head drifts subtly ±5° pan / ±4° tilt

**Captions**
- Multi-line word-wrap (40 chars/line, up to 12 lines), paginated every 3.5s
- TEXT_LIVE holds 60s — every keystroke on the Pi appears on Pip's face in real time

**Touch**
- Poke the screen → one of 6 random flinch reactions (emotion + caption + servo recoil)
- 1.5s cooldown between pokes

---

## Project Files

| File | Purpose |
|---|---|
| `AGENTS.md` | Master context — read before any session |
| `HARDWARE.md` | Wiring diagrams and component reference |
| `HOUSING.md` | Robot body build guide |
| `config.py` | All settings, pin numbers, color maps |
| `brain.py` | AI conversation core |
| `face.py` | Pi-side serial wrapper for the ESP32 face |
| `requirements.txt` | Python dependencies |
| `setup.sh` | One-shot Pi setup script |
| `firmware/face/` | PlatformIO project — ESP32 face firmware |
| `firmware/face/src/main.cpp` | Firmware entry point + serial command dispatch |
| `firmware/face/src/emotions.h` | 12-emotion shape + eyebrow table |
| `firmware/face/src/state.{h,cpp}` | Animation state machine + idle behaviors |
| `firmware/face/src/face_render.{h,cpp}` | PSRAM framebuffer + drawing primitives |
| `firmware/face/src/serial_parser.{h,cpp}` | Line-based ASCII command parser |
| `firmware/face/src/servos.{h,cpp}` | Two-servo pan-tilt with smooth interpolation |
| `firmware/face/src/touch.{h,cpp}` | CST816 capacitive touch polling |
| `tests/test_face.py` | pytest suite for face.py (MockSerial) |

---

## Build Stages

| Stage | Goal | Status |
|---|---|---|
| 1 | Brain — AI API conversation | ✅ Done |
| 2 | Face — animated round display, captions, touch | ✅ Done++ |
| 3a | Audio out — TTS to speaker | ⏸ Paused (hardware — need amplifier) |
| 3b | Audio in — mic + STT | ⬜ Planned (Nano 33 BLE Sense remote) |
| 4 | Movement — servos | ✅ Essentially done early (2-servo pan-tilt on ESP32) |
| 4.5 | Extended movement — PCA9685 for more axes | ⬜ Pending |
| 5 | Polish — LEDs, wake word, housing | ⬜ Pending |

**Stage 3a status:** PCM5102 DAC wired and tested, but it doesn't drive passive speakers loud enough without an amplifier. Shopping for a MAX98357A (I²S amp, ~$5) or powered speaker before proceeding. The wiring knowledge is solid — it's a hardware gap only.

**Stage 3b plan:** Arduino Nano 33 BLE Sense Rev2 (already owned) will become a handheld mic remote. Its built-in PDM mic + IMU + BLE makes it ideal for capturing clean voice close to the user.

**Stage 4 status:** Two-servo pan-tilt delivered as part of Stage 2 polish. Servos run directly from the ESP32-S3 (no PCA9685 needed for just 2). PCA9685 is parked for future multi-servo expansion (Stage 4.5+).

---

## Hardware

See `HARDWARE.md` for full wiring. Key components:

- **Brain:** Raspberry Pi 5 8GB (CanaKit)
- **Face:** Waveshare ESP32-S3-Touch-LCD-2.1 (integrated ESP32-S3 + ST7701S 480×480 round LCD + CST816 touch)
- **Pi ↔ Face link:** Single USB-C cable (Pi `/dev/ttyACM0` ↔ ESP32 native USB-OTG)
- **Head servos:** 2x SG90 wired directly to ESP32-S3 GPIO43/44 (working now)
- **Mic:** INMP441 I²S microphone (Stage 3b, via Nano 33 remote)
- **Speaker/DAC:** CQRobot 3W + PCM5102 DAC (paused — needs amp)
- **Future servos:** SunFounder PCA9685 + XiaoR Geek pan-tilt kit (Stage 4.5)
- **Future handheld remote:** Arduino Nano 33 BLE Sense Rev2 (Stage 3b)
