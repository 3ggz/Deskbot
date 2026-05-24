"""Unit tests for face.py — the Pi-side serial wrapper for the ESP32 face controller."""
import logging
import pytest
from face import Face


def test_face_handles_missing_port_gracefully(caplog):
    """If the ESP32 isn't plugged in, Face must not crash brain.py."""
    with caplog.at_level(logging.WARNING):
        face = Face(port="/dev/does-not-exist-xyz", baud=115200)
    assert face.is_connected() is False
    # All methods must be safe to call even when disconnected
    face.set_emotion("happy", "green")
    face.blink()
    face.reset()
    # The reviewer-recommended assertion: verify the warning was actually logged
    assert any("does-not-exist-xyz" in record.message for record in caplog.records), \
        "Face must log a warning when connection fails, not silently swallow it"
    face.close()


class MockSerial:
    """Test double for serial.Serial — captures bytes written."""
    def __init__(self, *args, **kwargs):
        self.is_open = True
        self.written = bytearray()

    def write(self, data):
        self.written.extend(data)

    def flush(self):
        pass

    def close(self):
        self.is_open = False


def test_set_emotion_sends_correct_command(monkeypatch):
    mock = MockSerial()
    monkeypatch.setattr("face.serial.Serial", lambda *a, **kw: mock)
    face = Face(port="/dev/fake", baud=115200)
    face.set_emotion("excited", "#FFD700")
    assert mock.written == b"EMOTION excited #FFD700\n"


def test_blink_sends_blink_command(monkeypatch):
    mock = MockSerial()
    monkeypatch.setattr("face.serial.Serial", lambda *a, **kw: mock)
    face = Face(port="/dev/fake", baud=115200)
    face.blink()
    assert mock.written == b"BLINK\n"


def test_reset_sends_reset_command(monkeypatch):
    mock = MockSerial()
    monkeypatch.setattr("face.serial.Serial", lambda *a, **kw: mock)
    face = Face(port="/dev/fake", baud=115200)
    face.reset()
    assert mock.written == b"RESET\n"


def test_face_remembers_last_emotion(monkeypatch):
    mock = MockSerial()
    monkeypatch.setattr("face.serial.Serial", lambda *a, **kw: mock)
    face = Face(port="/dev/fake", baud=115200)
    face.set_emotion("sleepy", "#888888")
    assert face._last_emotion == "sleepy"
    assert face._last_color_hex == "#888888"
