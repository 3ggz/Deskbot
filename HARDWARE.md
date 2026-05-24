# HARDWARE.md — Wiring Reference

## Pi 5 GPIO Pinout Quick Reference
```
3.3V  → Pin 1, 17
5V    → Pin 2, 4
GND   → Pin 6, 9, 14, 20, 25, 30, 34, 39
SDA   → Pin 3  (GPIO 2)
SCL   → Pin 5  (GPIO 3)
```

---

## Face Display — Waveshare ESP32-S3-Touch-LCD-2.1

This is **not** a Pi-driven SPI display. It's a self-contained dev board. The ESP32-S3 on the board drives the LCD directly via an internal parallel RGB bus. The Pi doesn't touch any display pins.

**What's on the board (all internal, no Pi wiring needed):**
| Component | Details |
|---|---|
| Microcontroller | ESP32-S3, dual-core 240MHz, 8MB Flash, 8MB PSRAM (OPI) |
| LCD | 2.1" round, 480×480, ST7701S driver (parallel RGB) |
| Touch | CST816S capacitive touch controller (I²C internal) |
| IO Expander | TCA9554PWR — routes LCD RST, CS, backlight, buzzer |
| Buzzer | Piezo on TCA9554 EXIO_PIN8 — firmware holds it LOW (silent) |

**Connecting to the Pi:**
```
Waveshare USB port → USB-C cable → Pi USB-A port
                                   appears as /dev/ttyACM0
```

**Important — there are two USB-C ports on the board:**
| Port label | Function | Use this one? |
|---|---|---|
| USB | Native USB-OTG (ESP32-S3 CDC) | ✅ YES — this is how brain.py talks to the firmware |
| UART | USB-to-UART bridge (CH343P) | For PlatformIO flash/monitor from Windows only |

Use the **USB** port for runtime communication with the Pi. The Pi will see it as `/dev/ttyACM0`.

**Flashing firmware (from Windows development machine):**
```bash
cd firmware/face
pio run -t upload      # uses UART port, or USB with the right upload_port setting
pio device monitor     # confirm "READY v0.7"
```

**Library and driver notes:**
- Arduino-GFX 1.4.9 is pinned — the `espressif32` PlatformIO platform ships Arduino-ESP32 2.0.17, and Arduino-GFX 1.5+ requires 3.x
- LovyanGFX was evaluated but neither it nor Arduino-GFX drives the TCA9554 IO expander out of the box
- Display init uses ESP-IDF's `esp_lcd_new_rgb_panel()` directly, adapted from: https://gist.github.com/fallenartist/22d1d01e125afb02ae4ebdcdf02d1f80

---

## Microphone — INMP441 (I²S) — Stage 3

```
Mic Pin  → Pi Pin    (GPIO)
VDD      → Pin 1     (3.3V)
GND      → Pin 6     (GND)
SCK      → Pin 12    (GPIO 18)
WS       → Pin 35    (GPIO 19)
SD       → Pin 38    (GPIO 20)
L/R      → GND       (selects left channel)
```
**Library:** `sounddevice` or `pyaudio` with ALSA I²S driver.
Requires adding to `/boot/config.txt`:
```
dtoverlay=googlevoicehat-soundcard
# or
dtoverlay=i2s-mmap
```

---

## DAC / Speaker — PCM5102 + CQRobot 3W Speaker — Stage 3

```
PCM5102 Pin → Pi Pin    (GPIO)
VIN         → Pin 2     (5V)
GND         → Pin 6     (GND)
BCK         → Pin 12    (GPIO 18)
LRCK/WS     → Pin 35    (GPIO 19)
DIN         → Pin 40    (GPIO 21)
FMT         → GND       (I²S format)
DEMP        → GND       (de-emphasis off)
XSMT        → 3.3V      (unmute)
```
Speaker wires → PCM5102 OUTL/OUTR or mono bridged output.

**Note:** Mic (INMP441) and DAC (PCM5102) share I²S bus pins. This is intentional — Pi 5 supports full-duplex I²S. Configure as a single soundcard. Resolve this properly in Stage 3 before soldering anything.

---

## Servo Driver — PCA9685 (I²C) — Stage 4

```
PCA9685 Pin → Pi Pin    (GPIO)
VCC         → Pin 1     (3.3V) ← logic power only
GND         → Pin 6     (GND)
SDA         → Pin 3     (GPIO 2)
SCL         → Pin 5     (GPIO 3)
V+          → External 5V supply (servo power — use breadboard PSU)
```
**Critical:** Do NOT power servos from Pi 3.3V or 5V pins.
Use the SunFounder BreadVolt or Arduino breadboard PSU for servo power.
Common GND between Pi and servo PSU is required.

**Servo connections to PCA9685:**
```
Channel 0 → Head pan servo (SG90)
Channel 1 → Head tilt servo (SG90)
Channel 2 → Body wiggle servo (SG90)
Channel 3 → Spare SG90
```
**Library:** `adafruit-circuitpython-pca9685` + `adafruit-circuitpython-motor`

---

## LED Strip — 5V USB LED Strip — Stage 5

```
Option A: Plug into Pi USB-A port (on/off only via USB power)
Option B: Wire data line to GPIO for addressable control
```
If the strip is addressable (WS2812B/NeoPixel type):
```
Data → GPIO 10 (Pin 19) ← or any free GPIO with PWM
Power → 5V external (NOT from Pi)
GND → Common GND
```
**Library (if addressable):** `rpi_ws281x`

---

## Logic Level Shifting

The Pi 5 GPIO is 3.3V logic. The face display no longer connects to Pi GPIO at all — that simplifies Stage 2 considerably.

| Component | Pi Voltage | Component Voltage | Needs Shifter? |
|---|---|---|---|
| ESP32-S3-Touch-LCD-2.1 | USB (5V bus) | 3.3V logic on ESP32 | No — USB handles it |
| INMP441 mic | 3.3V | 3.3V | No |
| PCM5102 DAC | 3.3V | 3.3V | No |
| PCA9685 | 3.3V | 3.3V | No |
| SG90 signal | 3.3V | 3.3V–5V | Maybe (usually fine) |
| Arduino UNO R4 serial | 3.3V | 5V | **Yes** |

**Recommendation:** One bidirectional logic level shifter (~$3) covers any 5V interfaces.

---

## I²C Bus Check

After wiring PCA9685 (Stage 4), verify it's detected:
```bash
sudo i2cdetect -y 1
```
PCA9685 default address: `0x40`
If not detected: check SDA/SCL wiring and that I²C is enabled in raspi-config.

---

## Known Hardware Quirks

**Buzzer on ESP32-S3-Touch-LCD-2.1:**
The board has a piezo buzzer wired to TCA9554 EXIO_PIN8. The Waveshare demo confirms this. The firmware explicitly drives EXIO_PIN8 LOW on startup to keep it silent.

If you hear intermittent buzzing after handling the board (especially after dropping it), that is a **hardware issue** — likely a cracked solder joint near the buzzer component — not a firmware bug. Inspect the board under magnification and reflow the buzzer solder joints if needed.

---

## Full Wiring Checklist

### Stage 2 (Face) — ready to run
- [x] ESP32-S3-Touch-LCD-2.1 USB-C connected to Pi USB-A
- [x] Firmware flashed and confirmed `READY v0.7` on serial monitor
- [x] face.py auto-connects on Pi at `/dev/ttyACM0`

### Stage 3 (Voice) — not yet
- [ ] INMP441 wired, I²S driver configured
- [ ] PCM5102 wired, speaker connected
- [ ] Full-duplex soundcard configured in `/boot/config.txt`

### Stage 4 (Movement) — not yet
- [ ] PCA9685 wired, servo PSU connected, common GND confirmed
- [ ] Servos plugged into PCA9685 channels 0–3
- [ ] Logic level shifter in place where needed
- [ ] i2cdetect shows 0x40 before running any servo code

### Stage 5 (LEDs/Polish) — not yet
- [ ] LED strip power from external 5V (not Pi pins)
