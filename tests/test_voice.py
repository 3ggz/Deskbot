"""Unit tests for voice.py — OpenAI TTS wrapper."""
import logging
import pytest


def test_voice_handles_missing_api_key_gracefully(caplog):
    """No API key → Voice is offline but doesn't crash."""
    from voice import Voice
    with caplog.at_level(logging.WARNING):
        v = Voice(api_key="")
    assert v.is_available() is False
    # All methods safe to call when offline
    v.say("Hello")
    v.stop()
    v.close()


def test_voice_handles_placeholder_api_key_gracefully():
    """The default '.env' placeholder string is treated as offline."""
    from voice import Voice
    v = Voice(api_key="your_openai_api_key_here")
    assert v.is_available() is False
    v.say("test")
    v.close()


def test_voice_say_empty_string_no_op():
    """Empty/whitespace text doesn't trigger a TTS call even if available."""
    from voice import Voice
    v = Voice(api_key="")  # offline anyway
    v.say("")
    v.say("   ")
    v.say(None)
    v.close()
