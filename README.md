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
pio device monitor      # confirm "READY v0.7" on boot
```

**On Pi (runtime):**
```bash
source venv/bin/activate
python3 brain.py        # Face auto-connects at /dev/ttyACM0
```

Plug the ESP32 into the Pi via USB-C. The face connects automatically — brain.py runs fine without it if it's not plugged in.

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
| `firmware/face/src/emotions.h` | 7-emotion shape table |
| `firmware/face/src/state.{h,cpp}` | Animation state machine + idle behaviors |
| `firmware/face/src/face_render.{h,cpp}` | PSRAM framebuffer + drawing primitives |
| `firmware/face/src/serial_parser.{h,cpp}` | Line-based ASCII command parser |
| `tests/test_face.py` | pytest suite for face.py (MockSerial) |

---

## Build Stages

| Stage | Goal | Status |
|---|---|---|
| 1 | Brain — AI API conversation | ✅ Done |
| 2 | Face — animated round display | 🔄 ~95% done (Tasks 17, 18 remain) |
| 3 | Voice — mic + speaker pipeline | ⬜ Pending |
| 4 | Movement — servos via PCA9685 | ⬜ Pending |
| 5 | Polish — LEDs, wake word, housing | ⬜ Pending |

**Stage 2 remaining:** Task 17 (Pi-side READY-resync — auto-recover on ESP32 unplug/replug) and Task 18 (end-to-end integration test on the Pi with brain.py + face together).

---

## Hardware

See `HARDWARE.md` for full wiring. Key components:

- **Brain:** Raspberry Pi 5 8GB (CanaKit)
- **Face:** Waveshare ESP32-S3-Touch-LCD-2.1 (integrated ESP32-S3 + ST7701S 480×480 round LCD)
- **Pi ↔ Face link:** Single USB-C cable (Pi `/dev/ttyACM0` ↔ ESP32 native USB-OTG)
- **Mic:** INMP441 I²S microphone (Stage 3)
- **Speaker:** CQRobot 3W + PCM5102 DAC (Stage 3)
- **Servos:** 4x SG90 + PCA9685 driver (Stage 4)
- **Neck:** XiaoR Geek pan-tilt kit (Stage 4)
