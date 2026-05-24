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

## Display — Waveshare 2.1" Round LCD (480×480, SPI)
```
Display Pin → Pi Pin    (GPIO)
VCC        → Pin 1     (3.3V)
GND        → Pin 6     (GND)
DIN/MOSI   → Pin 19    (GPIO 10)
CLK/SCLK   → Pin 23    (GPIO 11)
CS         → Pin 24    (GPIO 8)
DC         → Pin 22    (GPIO 25)
RST        → Pin 13    (GPIO 27)
BL         → Pin 12    (GPIO 18) ← PWM backlight
```
**Library:** `st7789` or Waveshare's own Python demo
**Note:** Uses SPI0. SPI must be enabled via `raspi-config`.

---

## Microphone — INMP441 (I²S)
```
Mic Pin  → Pi Pin    (GPIO)
VDD      → Pin 1     (3.3V)
GND      → Pin 6     (GND)
SCK      → Pin 12    (GPIO 18) ← shared with BL, see note
WS       → Pin 35    (GPIO 19)
SD       → Pin 38    (GPIO 20)
L/R      → GND       (selects left channel)
```
**Note on GPIO 18 conflict:** GPIO 18 is used for both display backlight PWM and I²S clock.
Resolution options:
1. Move backlight to software control (always on, no PWM) — simplest
2. Use GPIO 13 (Pin 33) for backlight instead — recommended
3. Time-multiplex (only mic OR audio active at once) — complex

**Recommended fix:** Wire BL to GPIO 13 instead, free GPIO 18 fully for I²S.

**Library:** `sounddevice` or `pyaudio` with ALSA I²S driver.
Requires adding to `/boot/config.txt`:
```
dtoverlay=googlevoicehat-soundcard
# or
dtoverlay=i2s-mmap
```

---

## DAC / Speaker — PCM5102 + CQRobot 3W Speaker
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

**Note:** Mic (INMP441) and DAC (PCM5102) share I²S bus pins.
This is intentional — they use different I²S channels and directions.
The Pi 5 supports full-duplex I²S. Configure as a single soundcard.

---

## Servo Driver — PCA9685 (I²C)
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

## LED Strip — 5V USB LED Strip
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
The Pi 5 GPIO is 3.3V logic. Some components need level shifting:

| Component | Pi Voltage | Component Voltage | Needs Shifter? |
|---|---|---|---|
| INMP441 mic | 3.3V | 3.3V | No |
| PCM5102 DAC | 3.3V | 3.3V | No |
| Waveshare display | 3.3V | 3.3V | No |
| PCA9685 | 3.3V | 3.3V | No |
| SG90 signal | 3.3V | 3.3V–5V | Maybe (usually fine) |
| Arduino UNO R4 serial | 3.3V | 5V | **Yes** |

**Recommendation:** Get one bidirectional logic level shifter (~$3) for any 5V interfaces.

---

## I²C Bus Check
After wiring PCA9685, verify it's detected:
```bash
sudo i2cdetect -y 1
```
PCA9685 default address: `0x40`
If not detected: check SDA/SCL wiring and that I²C is enabled in raspi-config.

---

## Full Wiring Checklist (Complete Before Powering On)
- [ ] Display wired and SPI enabled
- [ ] PCA9685 wired, servo PSU connected, common GND confirmed
- [ ] Servos plugged into PCA9685 channels 0–3
- [ ] INMP441 wired with GPIO 18 conflict resolved
- [ ] PCM5102 wired, speaker connected
- [ ] LED strip power from external 5V (not Pi pins)
- [ ] Logic level shifter in place where needed
- [ ] i2cdetect shows 0x40 before running any servo code
