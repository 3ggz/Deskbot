# Desk Companion Bot

A small, expressive AI-powered robot that sits on your desk. Animated round face, physical head movement, voice conversation, mood lighting.

**Read `AGENTS.md` first** — it contains the full project context, hardware inventory, architecture decisions, and build stages.

---

## Quick Start (Stage 1 — Brain Only)

```bash
# 1. Clone / copy this folder to your Pi
# 2. Run setup
bash scripts/setup.sh

# 3. Add your API key
nano .env  # Set ANTHROPIC_API_KEY

# 4. Reboot (required for I2S)
sudo reboot

# 5. After reboot — run Stage 1
source venv/bin/activate
cd src && python3 brain.py
```

If the bot talks back in the terminal with emotion tags — Stage 1 is done. ✅

---

## Project Files

| File | Purpose |
|---|---|
| `AGENTS.md` | Master context — read before any session |
| `HARDWARE.md` | Wiring diagrams and pin assignments |
| `HOUSING.md` | Robot body build guide |
| `src/config.py` | All settings and pin numbers |
| `src/brain.py` | AI conversation core |
| `src/main.py` | Full system entry point (Stage 5+) |
| `scripts/setup.sh` | One-shot Pi setup script |

---

## Build Stages

| Stage | Goal | Status |
|---|---|---|
| 1 | Brain — AI API conversation | 🔄 In progress |
| 2 | Face — animated round display | ⬜ Pending |
| 3 | Voice — mic + speaker pipeline | ⬜ Pending |
| 4 | Movement — servos via PCA9685 | ⬜ Pending |
| 5 | Polish — LEDs, wake word, housing | ⬜ Pending |

---

## Hardware

See `HARDWARE.md` for full wiring. Key components:

- **Brain:** Raspberry Pi 5 8GB (CanaKit)
- **Face:** Waveshare 2.1" Round LCD 480×480
- **Mic:** INMP441 I²S microphone
- **Speaker:** CQRobot 3W + PCM5102 DAC
- **Servos:** 4x SG90 + PCA9685 driver
- **Neck:** XiaoR Geek pan-tilt kit
