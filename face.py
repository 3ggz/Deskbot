"""face.py — Pi-side serial wrapper for the ESP32 face controller.

Sends line-based ASCII commands like `EMOTION happy #00FF00\n` to the
ESP32-S3-Touch-LCD-2.1 over USB serial. Gracefully no-ops if the
controller isn't plugged in, so brain.py keeps working without the
face hardware.
"""
import logging
import threading
import serial

log = logging.getLogger(__name__)


class Face:
    def __init__(self, port: str = "/dev/ttyACM0", baud: int = 115200, timeout: float = 0.1):
        self._port = port
        self._baud = baud
        self._serial = None
        self._lock = threading.Lock()
        self._last_emotion = None
        self._last_color_hex = None
        try:
            self._serial = serial.Serial(port=port, baudrate=baud, timeout=timeout)
            log.info(f"Face connected on {port} @ {baud}")
        except (serial.SerialException, FileNotFoundError, OSError) as e:
            log.warning(f"Face not connected ({port}): {e}. Running in offline mode.")
            self._serial = None

    def is_connected(self) -> bool:
        return self._serial is not None and self._serial.is_open

    def _send_line(self, line: str) -> None:
        if not self.is_connected():
            return
        with self._lock:
            try:
                self._serial.write(f"{line}\n".encode("ascii"))
                self._serial.flush()
            except (serial.SerialException, OSError) as e:
                log.warning(f"Face write failed: {e}")

    def set_emotion(self, emotion: str, led_color: str) -> None:
        # led_color may be a hex string like "#00FF00" or a name we look up
        # For now, accept hex strings; brain.py converts color names before calling.
        self._last_emotion = emotion
        self._last_color_hex = led_color
        self._send_line(f"EMOTION {emotion} {led_color}")

    def blink(self) -> None:
        self._send_line("BLINK")

    def reset(self) -> None:
        self._send_line("RESET")

    def close(self) -> None:
        if self._serial is not None and self._serial.is_open:
            self._serial.close()
            log.info("Face serial port closed")
