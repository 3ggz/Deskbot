"""Unit tests for display.py — the rectangular conversation-history display."""
import logging
import pytest
from unittest.mock import patch, MagicMock


def test_display_handles_no_hardware_gracefully(caplog):
    """If spidev/RPi.GPIO aren't available, Display must not crash."""
    with patch("display._HAS_HARDWARE", False):
        from display import Display
        with caplog.at_level(logging.WARNING):
            d = Display()
        assert d.is_connected() is False
        # All operations must be safe even when offline
        d.add_message("user", "hello")
        d.add_message("pip", "hi there")
        d.update_status("happy", "#00FF00")
        d.render()
        d.clear()
        d.close()


def test_history_caps_at_max():
    """Adding more than MAX_HISTORY messages drops oldest."""
    from display import Display
    d = Display()  # offline mode (assuming no hardware on dev machine)
    for i in range(20):
        d.add_message("user", f"msg {i}")
    assert len(d.history) <= Display.MAX_HISTORY


def test_status_color_hex_parsing():
    """Status color hex strings parse correctly."""
    from display import Display
    d = Display()
    assert d._hex_to_rgb("#FF0000") == (255, 0, 0)
    assert d._hex_to_rgb("#00FF00") == (0, 255, 0)
    assert d._hex_to_rgb("#0000FF") == (0, 0, 255)
    assert d._hex_to_rgb("invalid") == (200, 200, 200)


def test_text_truncation_long_messages():
    """Messages longer than ~200 chars get truncated."""
    from display import Display
    d = Display()
    long_text = "x" * 500
    d.add_message("user", long_text)
    role, stored = d.history[0]
    assert len(stored) <= 200
