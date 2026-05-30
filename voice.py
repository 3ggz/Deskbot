"""voice.py — OpenAI TTS wrapper. After each AI response, Pip speaks his
reply through the Pi's default audio device.

The actual audio playback runs in a background subprocess so the input
prompt doesn't block while Pip is talking. A new message interrupts
the previous speech (kills the playback subprocess, starts a new one).

Gracefully no-ops if OPENAI_API_KEY isn't set, aplay isn't installed,
or the OpenAI API call fails — Pip just goes silent. Conversation
continues normally.
"""
import logging
import os
import subprocess
import tempfile
import threading

log = logging.getLogger(__name__)

try:
    from openai import OpenAI
    _HAS_OPENAI = True
except ImportError:
    _HAS_OPENAI = False


class Voice:
    """OpenAI TTS playback wrapper."""

    def __init__(self, api_key: str = "", model: str = "tts-1",
                 voice: str = "nova", playback_cmd: str = "aplay"):
        self.api_key = api_key
        self.model = model
        self.voice = voice
        self.playback_cmd = playback_cmd

        self.client = None
        self._current_proc = None
        self._lock = threading.Lock()

        if not _HAS_OPENAI:
            log.warning("openai library not available — Voice running in offline mode")
            return
        if not api_key or api_key == "your_openai_api_key_here":
            log.warning("OPENAI_API_KEY not set — Voice running in offline mode")
            return

        try:
            self.client = OpenAI(api_key=api_key)
            log.info(f"Voice initialized. Model: {model}, voice: {voice}.")
        except Exception as e:
            log.warning(f"Voice setup failed: {e}. Running in offline mode.")
            self.client = None

    def is_available(self) -> bool:
        return self.client is not None

    def stop(self):
        """Kill any current playback subprocess immediately."""
        with self._lock:
            if self._current_proc is not None and self._current_proc.poll() is None:
                try:
                    self._current_proc.terminate()
                    self._current_proc.wait(timeout=0.5)
                except Exception:
                    try:
                        self._current_proc.kill()
                    except Exception:
                        pass
            self._current_proc = None

    def say(self, text: str):
        """Generate TTS audio + play it. Returns immediately (playback runs in background).
        Killing any in-progress playback first."""
        if not self.is_available() or not text or not text.strip():
            return
        self.stop()  # interrupt any in-flight playback

        # Generate + play in a worker thread so the API call doesn't block the caller.
        threading.Thread(target=self._generate_and_play, args=(text,), daemon=True).start()

    def _generate_and_play(self, text: str):
        try:
            # Call OpenAI TTS — wav format so aplay can play it directly without conversion
            response = self.client.audio.speech.create(
                model=self.model,
                voice=self.voice,
                input=text,
                response_format="wav",
            )
            audio_bytes = response.content
        except Exception as e:
            log.warning(f"TTS API call failed: {e}")
            return

        # Save to temp file
        try:
            tmp = tempfile.NamedTemporaryFile(delete=False, suffix=".wav", prefix="pip_speech_")
            tmp.write(audio_bytes)
            tmp.close()
            tmp_path = tmp.name
        except Exception as e:
            log.warning(f"TTS temp file failed: {e}")
            return

        # Play via aplay (default device — set by ~/.asoundrc)
        try:
            with self._lock:
                self._current_proc = subprocess.Popen(
                    [self.playback_cmd, "-q", tmp_path],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )
        except FileNotFoundError:
            log.warning(f"Playback command '{self.playback_cmd}' not found")
            try:
                os.unlink(tmp_path)
            except Exception:
                pass
            return
        except Exception as e:
            log.warning(f"Audio playback failed: {e}")
            try:
                os.unlink(tmp_path)
            except Exception:
                pass
            return

        # Wait for playback to finish, then clean up
        try:
            self._current_proc.wait()
        except Exception:
            pass
        try:
            os.unlink(tmp_path)
        except Exception:
            pass

    def close(self):
        self.stop()
