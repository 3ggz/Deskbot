"""display.py — driver and high-level wrapper for the Hosyond 4.0" ILI9486
SPI display (480x320), used as Pip's conversation-history screen.

Sits below the face screen physically. Shows the recent conversation, current
mood color, and Pip's emotion as a status bar.

Gracefully no-ops if the display isn't connected — same pattern as face.py.
"""
import logging
import time
from PIL import Image, ImageDraw, ImageFont

log = logging.getLogger(__name__)

# Soft import — display is optional hardware
try:
    import spidev
    import RPi.GPIO as GPIO
    _HAS_HARDWARE = True
except ImportError:
    _HAS_HARDWARE = False


# ─── ILI9486 low-level driver ──────────────────────────────────────────────

class ILI9486:
    """Minimal ILI9486 SPI driver. 480x320 landscape, 16-bit RGB565 pixel format."""

    # Landscape dimensions
    WIDTH = 480
    HEIGHT = 320

    def __init__(self, spi_bus=0, spi_device=0, dc_pin=25, rst_pin=24,
                 spi_speed_hz=40_000_000):
        if not _HAS_HARDWARE:
            raise RuntimeError("spidev / RPi.GPIO not available on this platform")

        self.dc_pin = dc_pin
        self.rst_pin = rst_pin

        # SPI bus
        self.spi = spidev.SpiDev()
        self.spi.open(spi_bus, spi_device)
        self.spi.max_speed_hz = spi_speed_hz
        self.spi.mode = 0

        # GPIO setup
        GPIO.setwarnings(False)
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(dc_pin, GPIO.OUT)
        GPIO.setup(rst_pin, GPIO.OUT)

        self._reset()
        self._init_display()

    def _reset(self):
        GPIO.output(self.rst_pin, GPIO.HIGH)
        time.sleep(0.005)
        GPIO.output(self.rst_pin, GPIO.LOW)
        time.sleep(0.020)
        GPIO.output(self.rst_pin, GPIO.HIGH)
        time.sleep(0.150)

    def _cmd(self, cmd):
        GPIO.output(self.dc_pin, GPIO.LOW)
        self.spi.writebytes([cmd])

    def _data(self, data):
        GPIO.output(self.dc_pin, GPIO.HIGH)
        if isinstance(data, int):
            self.spi.writebytes([data])
        else:
            self.spi.writebytes(list(data))

    def _init_display(self):
        # ILI9486 init sequence — adapted from common open-source drivers
        self._cmd(0x01); time.sleep(0.120)  # software reset
        self._cmd(0x11); time.sleep(0.020)  # sleep out

        # Interface pixel format = 16-bit RGB565
        self._cmd(0x3A); self._data(0x55)

        # Power control
        self._cmd(0xC0); self._data([0x0E, 0x0E])
        self._cmd(0xC1); self._data([0x41, 0x00])
        self._cmd(0xC2); self._data([0x55])
        self._cmd(0xC5); self._data([0x00, 0x00, 0x00, 0x00])

        # Positive / negative gamma (typical ILI9486 values)
        self._cmd(0xE0); self._data([0x0F, 0x1F, 0x1C, 0x0C, 0x0F, 0x08, 0x48, 0x98,
                                     0x37, 0x0A, 0x13, 0x04, 0x11, 0x0D, 0x00])
        self._cmd(0xE1); self._data([0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47, 0x75,
                                     0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00])

        # Frame rate
        self._cmd(0xB1); self._data([0xA0, 0x11])
        # Display inversion off
        self._cmd(0x20)
        # Display function control
        self._cmd(0xB6); self._data([0x02, 0x02])

        # Memory access control — landscape orientation, RGB
        self._cmd(0x36); self._data(0x28)

        # Sleep out + display on
        self._cmd(0x11); time.sleep(0.150)
        self._cmd(0x29); time.sleep(0.020)

    def set_window(self, x0, y0, x1, y1):
        self._cmd(0x2A)
        self._data([(x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF])
        self._cmd(0x2B)
        self._data([(y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF])
        self._cmd(0x2C)

    def push_image(self, image):
        """Push a PIL Image (RGB or RGB565-compatible) to the screen.

        Image must be exactly WIDTH x HEIGHT (will be resized otherwise)."""
        if image.size != (self.WIDTH, self.HEIGHT):
            image = image.resize((self.WIDTH, self.HEIGHT))
        if image.mode != "RGB":
            image = image.convert("RGB")

        # Convert RGB888 -> RGB565 (big-endian, MSB first per the ILI9486)
        pixels = bytes(image.tobytes("raw", "RGB"))
        rgb565 = bytearray(len(pixels) // 3 * 2)
        idx = 0
        for i in range(0, len(pixels), 3):
            r, g, b = pixels[i], pixels[i + 1], pixels[i + 2]
            v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            rgb565[idx]     = (v >> 8) & 0xFF
            rgb565[idx + 1] = v & 0xFF
            idx += 2

        self.set_window(0, 0, self.WIDTH - 1, self.HEIGHT - 1)
        GPIO.output(self.dc_pin, GPIO.HIGH)
        # SPI buffer limit on Pi is typically 4096 bytes per write; chunk it.
        chunk = 4096
        for i in range(0, len(rgb565), chunk):
            self.spi.writebytes2(rgb565[i:i + chunk])

    def close(self):
        try:
            self.spi.close()
        except Exception:
            pass


# ─── High-level Display wrapper ────────────────────────────────────────────

# Try a few standard font paths — first found wins
_FONT_CANDIDATES = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "C:\\Windows\\Fonts\\arial.ttf",  # Windows dev box
]


def _load_font(size):
    for path in _FONT_CANDIDATES:
        try:
            return ImageFont.truetype(path, size)
        except (OSError, IOError):
            continue
    return ImageFont.load_default()


class Display:
    """High-level conversation-history display.

    Holds the last N exchanges in memory and re-renders the full screen
    whenever a new message arrives. Safe to call any method even when
    the hardware isn't connected."""

    MAX_HISTORY = 6  # how many (role, text) pairs to keep

    HEADER_COLOR = (77, 208, 255)    # cyan — Pip's identity color
    USER_COLOR   = (255, 220, 100)   # warm yellow
    PIP_COLOR    = (180, 235, 255)   # bright pale cyan
    STATUS_COLOR = (160, 160, 180)   # neutral grey
    BG_COLOR     = (0, 0, 0)

    def __init__(self, spi_bus=0, spi_device=0, dc_pin=25, rst_pin=24):
        self.lcd = None
        self.history = []          # list of (role, text)
        self.current_emotion = "neutral"
        self.current_color   = "#4DD0FF"

        # Fonts
        self.font_header = _load_font(20)
        self.font_body   = _load_font(16)
        self.font_status = _load_font(13)

        try:
            self.lcd = ILI9486(spi_bus=spi_bus, spi_device=spi_device,
                               dc_pin=dc_pin, rst_pin=rst_pin)
            log.info(f"Display connected on SPI{spi_bus}.{spi_device}")
            self.clear()
        except Exception as e:
            log.warning(f"Display not connected: {e}. Running in offline mode.")
            self.lcd = None

    def is_connected(self):
        return self.lcd is not None

    def add_message(self, role, text):
        """role: 'user' or 'pip'. Text auto-truncated and added to history."""
        if not text:
            return
        text = " ".join(text.split())[:200]
        self.history.append((role, text))
        self.history = self.history[-self.MAX_HISTORY:]
        self.render()

    def update_status(self, emotion, color_hex):
        """Update the status bar without adding to history."""
        self.current_emotion = emotion
        self.current_color = color_hex
        self.render()

    def clear(self):
        if not self.is_connected():
            return
        img = Image.new("RGB", (ILI9486.WIDTH, ILI9486.HEIGHT), self.BG_COLOR)
        self.lcd.push_image(img)

    def close(self):
        if self.lcd is not None:
            self.lcd.close()
            self.lcd = None

    # ─── Rendering ───────────────────────────────────────────────────────

    def render(self):
        """Re-render the full screen from current state."""
        if not self.is_connected():
            return
        img = Image.new("RGB", (ILI9486.WIDTH, ILI9486.HEIGHT), self.BG_COLOR)
        draw = ImageDraw.Draw(img)

        # ── Header: "PIP" + current emotion + mood swatch ──
        draw.text((12, 6), "PIP", font=self.font_header, fill=self.HEADER_COLOR)
        # Mood color swatch (right-aligned)
        swatch_color = self._hex_to_rgb(self.current_color)
        draw.rectangle([(ILI9486.WIDTH - 100, 8), (ILI9486.WIDTH - 14, 30)],
                       fill=swatch_color)
        # Emotion label centered between
        draw.text((90, 10), self.current_emotion, font=self.font_status, fill=self.STATUS_COLOR)
        # Divider line
        draw.line([(10, 36), (ILI9486.WIDTH - 10, 36)], fill=self.STATUS_COLOR, width=1)

        # ── Conversation body ──
        y = 46
        line_height = 22
        max_y = ILI9486.HEIGHT - 30  # leave room for footer

        for role, text in self.history:
            if role == "user":
                color = self.USER_COLOR
                prefix = "You: "
            else:
                color = self.PIP_COLOR
                prefix = "Pip: "
            lines = self._wrap_text(prefix + text, ILI9486.WIDTH - 20, self.font_body)
            for line in lines:
                if y > max_y:
                    break
                draw.text((12, y), line, font=self.font_body, fill=color)
                y += line_height
            y += 4  # small extra gap between messages
            if y > max_y:
                break

        # ── Footer: divider + history count ──
        draw.line([(10, ILI9486.HEIGHT - 24), (ILI9486.WIDTH - 10, ILI9486.HEIGHT - 24)],
                  fill=self.STATUS_COLOR, width=1)
        history_count = len(self.history)
        draw.text((12, ILI9486.HEIGHT - 20),
                  f"history: {history_count}/{self.MAX_HISTORY}",
                  font=self.font_status, fill=self.STATUS_COLOR)

        self.lcd.push_image(img)

    def _hex_to_rgb(self, hex_str):
        h = hex_str.lstrip("#")
        if len(h) != 6:
            return (200, 200, 200)
        try:
            return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))
        except ValueError:
            return (200, 200, 200)

    def _wrap_text(self, text, max_width_px, font):
        """Word-wrap text to fit within max_width_px when rendered with font."""
        words = text.split(" ")
        lines = []
        current = ""
        for word in words:
            test = (current + " " + word).strip() if current else word
            w = self._text_width(test, font)
            if w <= max_width_px:
                current = test
            else:
                if current:
                    lines.append(current)
                # Word itself too long — hard break
                while self._text_width(word, font) > max_width_px and len(word) > 1:
                    for i in range(len(word), 0, -1):
                        if self._text_width(word[:i], font) <= max_width_px:
                            lines.append(word[:i])
                            word = word[i:]
                            break
                    else:
                        break
                current = word
        if current:
            lines.append(current)
        return lines

    def _text_width(self, text, font):
        # Pillow >= 9.2 uses textbbox; older uses textsize
        try:
            l, t, r, b = font.getbbox(text)
            return r - l
        except AttributeError:
            return font.getsize(text)[0]
