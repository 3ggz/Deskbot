# src/config.py
# Desk Companion Bot — Configuration
# !! DO NOT COMMIT API KEYS TO GIT !!
# Use environment variables or a .env file for production

import os
from dotenv import load_dotenv

load_dotenv()  # Reads .env in the current working directory

# ─── AI / API ──────────────────────────────────────────────
ANTHROPIC_API_KEY = os.environ.get("ANTHROPIC_API_KEY", "YOUR_KEY_HERE")
AI_MODEL = "claude-haiku-4-5"  # Fast + cheap; swap to claude-sonnet-4-6 or claude-opus-4-7 for more depth
AI_MAX_TOKENS = 256            # Keep short — bot speaks in 1–2 sentences
AI_TEMPERATURE = 0.8           # Slight creativity, not too random

# ─── Bot Personality ───────────────────────────────────────
BOT_NAME = "Pip"               # Change this to whatever name you give the bot
BOT_WAKE_WORD = "hey pip"      # Wake word for voice activation

# ─── Display (SPI) ─────────────────────────────────────────
DISPLAY_WIDTH  = 480           # Waveshare 2.1" round
DISPLAY_HEIGHT = 480
DISPLAY_ROTATION = 0           # 0, 90, 180, 270
SPI_DC_PIN  = 25               # GPIO 25
SPI_CS_PIN  = 8                # GPIO 8 (CE0)
SPI_RST_PIN = 27               # GPIO 27
SPI_BL_PIN  = 13               # GPIO 13 (backlight — moved from 18 to avoid I2S conflict)
SPI_BUS     = 0                # SPI bus 0
SPI_DEVICE  = 0                # SPI device 0

# ─── Audio Input — INMP441 Mic (I²S) ───────────────────────
MIC_SAMPLE_RATE    = 16000     # 16kHz — good for speech recognition
MIC_CHANNELS       = 1         # Mono
MIC_CHUNK_SIZE     = 1024
MIC_DEVICE_INDEX   = None      # None = auto-detect, set to int if needed
VAD_THRESHOLD      = 500       # Voice activity detection threshold (tune this)
VAD_SILENCE_MS     = 1500      # ms of silence before stopping recording

# ─── Audio Output — PCM5102 DAC + Speaker ──────────────────
SPEAKER_SAMPLE_RATE  = 44100
SPEAKER_CHANNELS     = 2       # Stereo DAC, even for mono output
SPEAKER_DEVICE_INDEX = None    # None = auto-detect

# ─── Speech-to-Text ────────────────────────────────────────
# Options: "whisper_api", "google", "vosk" (offline)
STT_ENGINE = "whisper_api"
WHISPER_MODEL = "whisper-1"    # OpenAI Whisper API model

# ─── Text-to-Speech ────────────────────────────────────────
# Options: "pyttsx3" (offline), "google_tts", "elevenlabs", "openai_tts"
TTS_ENGINE = "pyttsx3"         # Start with offline, upgrade later
TTS_VOICE_RATE = 175           # Words per minute
TTS_VOICE_PITCH = 1.2          # Slight higher pitch for robot character

# ─── Servo / PCA9685 (I²C) ─────────────────────────────────
PCA9685_ADDRESS   = 0x40       # Default I²C address
PCA9685_FREQUENCY = 50         # 50Hz for SG90 servos
I2C_BUS = 1                    # Pi I²C bus 1 (SDA=GPIO2, SCL=GPIO3)

# Servo channel assignments
SERVO_PAN_CHANNEL    = 0       # Head left/right
SERVO_TILT_CHANNEL   = 1       # Head up/down
SERVO_WIGGLE_CHANNEL = 2       # Body wiggle
SERVO_SPARE_CHANNEL  = 3       # Spare

# Servo travel limits (tune per physical setup to avoid binding)
SERVO_PAN_CENTER  = 90         # degrees
SERVO_PAN_MIN     = 45
SERVO_PAN_MAX     = 135
SERVO_TILT_CENTER = 90
SERVO_TILT_MIN    = 60
SERVO_TILT_MAX    = 110
SERVO_WIGGLE_CENTER = 90
SERVO_WIGGLE_AMP    = 15       # degrees either side for wiggle

# ─── LED Strip ─────────────────────────────────────────────
LED_PIN         = 10           # GPIO pin for addressable LED data
LED_COUNT       = 30           # Number of LEDs in strip
LED_BRIGHTNESS  = 128          # 0–255

LED_COLORS = {
    "warm_white": (255, 220, 180),
    "blue":       (50,  100, 255),
    "red":        (255, 50,  50),
    "purple":     (180, 50,  255),
    "green":      (50,  220, 100),
    "yellow":     (255, 220, 50),
    "off":        (0,   0,   0),
}

# ─── Emotion → Display Mapping ─────────────────────────────
# Used by display.py to select which face animation to render
EMOTION_DISPLAY_MAP = {
    "happy":     {"eye_curve": "up",    "pupil_size": 1.0, "blink_rate": 3.0},
    "thinking":  {"eye_curve": "flat",  "pupil_size": 0.7, "blink_rate": 1.5},
    "sad":       {"eye_curve": "down",  "pupil_size": 0.8, "blink_rate": 2.0},
    "excited":   {"eye_curve": "up",    "pupil_size": 1.2, "blink_rate": 5.0},
    "neutral":   {"eye_curve": "flat",  "pupil_size": 1.0, "blink_rate": 3.0},
    "surprised": {"eye_curve": "wide",  "pupil_size": 1.3, "blink_rate": 1.0},
    "sleepy":    {"eye_curve": "half",  "pupil_size": 0.6, "blink_rate": 0.5},
}

# ─── Emotion → Movement Mapping ────────────────────────────
EMOTION_MOVEMENT_MAP = {
    "happy":     "nod",
    "thinking":  "tilt_left",
    "sad":       "idle",
    "excited":   "wiggle",
    "neutral":   "idle",
    "surprised": "tilt_right",
    "sleepy":    "idle",
}

# ─── Timing ────────────────────────────────────────────────
DISPLAY_FPS       = 30         # Target animation frame rate
IDLE_TIMEOUT_SEC  = 30         # Seconds before entering idle/sleepy state
BLINK_INTERVAL_SEC = 3.0       # Default blink interval

# ─── Debug ─────────────────────────────────────────────────
DEBUG_MODE         = True      # Set False for production
LOG_LEVEL          = "DEBUG"   # DEBUG, INFO, WARNING, ERROR
SKIP_HARDWARE      = False     # Set True to run brain only (no GPIO) on non-Pi machine

# ─── Face Controller (ESP32-S3-Touch-LCD-2.1) ──────────────
FACE_SERIAL_PORT = "/dev/ttyACM0"
FACE_SERIAL_BAUD = 115200

def _rgb_to_hex(rgb):
    return "#{:02X}{:02X}{:02X}".format(*rgb)

# Derived from LED_COLORS above — sent to the face as part of EMOTION commands
LED_COLORS_HEX = {name: _rgb_to_hex(rgb) for name, rgb in LED_COLORS.items()}

# ─── Conversation History Display (Hosyond 4.0" ILI9486 SPI, 480x320) ──────
DISPLAY_SPI_BUS    = 0
DISPLAY_SPI_DEVICE = 0       # CE0 — chip select pin 24 on Pi
DISPLAY_DC_PIN     = 25      # data/command select
DISPLAY_RST_PIN    = 24      # reset

# ─── Voice / TTS (OpenAI) ──────────────────────────────────
OPENAI_API_KEY = os.environ.get("OPENAI_API_KEY", "")
TTS_MODEL = "tts-1"       # "tts-1" (fast/cheap) or "tts-1-hd" (higher quality, slower)
TTS_VOICE = "nova"        # alloy, echo, fable, onyx, nova, shimmer
TTS_PLAYBACK_CMD = "aplay"
