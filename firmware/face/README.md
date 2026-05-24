# Pip Face Firmware

Runs on the ESP32-S3-Touch-LCD-2.1. Receives `EMOTION`/`BLINK`/`RESET`/`PING`
commands over USB serial from the Pi and renders Pip's face on the 480x480
round LCD.

## Build & flash

```bash
cd firmware/face
pio run -t upload         # compile and flash via the USB port
pio device monitor        # open serial monitor at 115200 baud
```

## Manual test from the command line

With the ESP32 plugged in, open a separate terminal:

```bash
echo "EMOTION happy #00FF00" > /dev/ttyACM0
```

The eyes should change.
