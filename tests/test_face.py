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
