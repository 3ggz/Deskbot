# src/brain.py
# Desk Companion Bot — AI Brain (Stage 1)
# This is the core module. Everything else plugs into this.
#
# Stage 1 goal: Typed conversation that returns structured JSON
# Run with: python3 brain.py
# No hardware required for this stage.

import os
import json
import logging
import random
import sys
import threading
import time
from anthropic import Anthropic
from face import Face
from display import Display
from voice import Voice

# Linux-only character-by-character input. Falls back gracefully on Windows.
try:
    import termios
    import tty
    import select
    HAS_TERMIOS = True
except ImportError:
    HAS_TERMIOS = False

def read_line_with_live_face(face, prompt="You: "):
    """Read a line of input from stdin, character-by-character, sending each
    intermediate buffer to face.say() so the user sees their text on Pip in real time.

    Returns the completed line (without the trailing newline) on Enter.
    Raises EOFError on Ctrl+D, KeyboardInterrupt on Ctrl+C.
    """
    sys.stdout.write(prompt)
    sys.stdout.flush()

    if not HAS_TERMIOS:
        # Windows or other non-Linux — fall back to regular input(). No live caption.
        return input()

    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    buffer = ""
    last_sent = None
    last_send_time = 0.0
    SEND_INTERVAL_S = 0.06  # don't spam more than ~16 updates/sec to serial

    try:
        tty.setcbreak(fd)
        while True:
            # Wait for a character (with small timeout so we can throttle sends)
            r, _, _ = select.select([fd], [], [], 0.05)
            if r:
                ch = sys.stdin.read(1)
                if ch == "\n" or ch == "\r":
                    sys.stdout.write("\n")
                    sys.stdout.flush()
                    # Clear the live caption so the actual response can take over
                    if face is not None:
                        face._send_line("TEXT_CLEAR")
                    return buffer
                elif ch in ("\x7f", "\x08"):  # backspace / DEL
                    if buffer:
                        buffer = buffer[:-1]
                        sys.stdout.write("\b \b")
                        sys.stdout.flush()
                elif ch == "\x03":  # Ctrl+C
                    sys.stdout.write("\n")
                    raise KeyboardInterrupt
                elif ch == "\x04":  # Ctrl+D
                    sys.stdout.write("\n")
                    raise EOFError
                elif ch.isprintable():
                    buffer += ch
                    sys.stdout.write(ch)
                    sys.stdout.flush()

            # Throttle face updates — only send if buffer changed and enough time passed
            now = time.time()
            if buffer != last_sent and (now - last_send_time) > SEND_INTERVAL_S:
                if face is not None:
                    # TEXT_LIVE holds the caption for 60s — won't auto-expire mid-typing
                    sanitized = " ".join(buffer.split())[:280]
                    face._send_line(f"TEXT_LIVE {sanitized}")
                last_sent = buffer
                last_send_time = now
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)


# ─── Setup ─────────────────────────────────────────────────
logging.basicConfig(level=logging.DEBUG)
log = logging.getLogger(__name__)

try:
    from config import (
        ANTHROPIC_API_KEY, AI_MODEL, AI_MAX_TOKENS,
        AI_TEMPERATURE, BOT_NAME,
        FACE_SERIAL_PORT, FACE_SERIAL_BAUD, LED_COLORS_HEX,
        DISPLAY_SPI_BUS, DISPLAY_SPI_DEVICE, DISPLAY_DC_PIN, DISPLAY_RST_PIN,
        OPENAI_API_KEY, TTS_MODEL, TTS_VOICE, TTS_PLAYBACK_CMD,
    )
except ImportError:
    # Fallback defaults if run standalone
    ANTHROPIC_API_KEY  = os.environ.get("ANTHROPIC_API_KEY", "")
    AI_MODEL           = "claude-haiku-4-5"
    AI_MAX_TOKENS      = 256
    AI_TEMPERATURE     = 0.8
    BOT_NAME           = "Pip"
    FACE_SERIAL_PORT   = "/dev/ttyACM0"
    FACE_SERIAL_BAUD   = 115200
    LED_COLORS_HEX     = {"warm_white": "#FFDCB4"}
    DISPLAY_SPI_BUS    = 0
    DISPLAY_SPI_DEVICE = 0
    DISPLAY_DC_PIN     = 25
    DISPLAY_RST_PIN    = 24
    OPENAI_API_KEY   = os.environ.get("OPENAI_API_KEY", "")
    TTS_MODEL        = "tts-1"
    TTS_VOICE        = "nova"
    TTS_PLAYBACK_CMD = "aplay"

# ─── System Prompt ─────────────────────────────────────────
SYSTEM_PROMPT = f"""You are {BOT_NAME}, a small friendly desktop robot companion.
You are witty, warm, curious, and occasionally playful. You live on the user's desk.
You have a physical body with a round screen face and you can move your head.

CRITICAL: Always respond ONLY with a single valid JSON object. No preamble, no markdown, no explanation.

Required format:
{{
  "emotion": "happy|thinking|sad|excited|neutral|surprised|sleepy|angry|love|confused|embarrassed|wink",
  "speech": "Your spoken response here (1-2 sentences max)",
  "movement": "nod|shake|tilt_left|tilt_right|wiggle|idle|dance|look_around|bow",
  "led_color": "warm_white|blue|red|purple|green|yellow"
}}

Emotion guide:
- neutral: factual responses, short answers — the default
- happy: general positive, helpful responses
- excited: good news, greetings, interesting topics, enthusiasm
- thinking: while answering complex or thoughtful questions
- surprised: unexpected info, plot twists, "whoa really?"
- sad: bad news, apologies, empathetic moments
- sleepy: late night, idle, low energy
- angry: frustration (rare — use sparingly), strong disagreement
- love: warm affection, compliments, fondness
- confused: when something doesn't make sense, "huh?"
- embarrassed: apologizing for a mistake, awkward situations
- wink: playful agreement, in-jokes, mischievous moments

Movement guide:
- nod: agreement, greeting, happy affirmation
- shake: disagreement, "no", confusion
- tilt_left: thinking, curious
- tilt_right: surprised, playful
- wiggle: a little excitement, mild celebration
- dance: BIG energy — wins, hype, "let's gooo", love, congratulations (use sparingly for impact!)
- look_around: contemplative, scanning, "let me think..."
- bow: gracious, polite, "you're welcome", "at your service"
- idle: neutral, no movement

LED color guide:
- warm_white: neutral, conversational
- blue: calm, informational, thinking
- purple: creative, playful
- green: positive, success
- yellow: curious, excited
- red: alert, important, sad

Never break character. Never output anything outside the JSON object.
"""

DREAM_PROMPT = """You are Pip, a small desktop companion robot, currently sleeping.
You're mumbling in your sleep — talking out a dream, a half-remembered fact, an absurd
thought, or a quiet observation.

Output ONE short sentence, under 12 words. It should feel sleepy and amusing — playful,
silly, mysterious, or oddly profound. NO preamble. NO quotes. NO JSON. Just the text.

Examples of the vibe:
- The clouds are made of cheese tonight...
- Octopuses don't carry receipts.
- mmm... seven... no, eight...
- I was Napoleon in 1923. Was I?
- Why does the toaster judge me?
- A perfect square of marshmallows.
- Trees know all my passwords.
- bleep... bloop... not now mom...
"""

# ─── Response Parsing ──────────────────────────────────────
DEFAULT_RESPONSE = {
    "emotion": "neutral",
    "speech": "Hmm, something went wrong. Let me think...",
    "movement": "idle",
    "led_color": "warm_white"
}

def parse_response(raw_text: str) -> dict:
    """
    Parse the AI response into a structured dict.
    Handles edge cases where the model adds extra text.
    """
    raw_text = raw_text.strip()

    # Try direct parse first
    try:
        parsed = json.loads(raw_text)
        return validate_response(parsed)
    except json.JSONDecodeError:
        pass

    # Try to extract JSON from within the text
    try:
        start = raw_text.index("{")
        end = raw_text.rindex("}") + 1
        parsed = json.loads(raw_text[start:end])
        return validate_response(parsed)
    except (ValueError, json.JSONDecodeError):
        log.warning(f"Could not parse response: {raw_text[:100]}")
        return DEFAULT_RESPONSE.copy()


def validate_response(data: dict) -> dict:
    """Ensure all required fields exist with valid values."""
    valid_emotions  = {"happy", "thinking", "sad", "excited", "neutral", "surprised", "sleepy",
                       "angry", "love", "confused", "embarrassed", "wink"}
    valid_movements = {"nod", "shake", "tilt_left", "tilt_right", "wiggle", "idle",
                       "dance", "look_around", "bow"}
    valid_colors    = {"warm_white", "blue", "red", "purple", "green", "yellow"}

    result = DEFAULT_RESPONSE.copy()

    if data.get("emotion") in valid_emotions:
        result["emotion"] = data["emotion"]
    if data.get("movement") in valid_movements:
        result["movement"] = data["movement"]
    if data.get("led_color") in valid_colors:
        result["led_color"] = data["led_color"]
    if isinstance(data.get("speech"), str) and data["speech"].strip():
        result["speech"] = data["speech"].strip()

    return result


# ─── Brain Class ───────────────────────────────────────────
class Brain:
    """
    Core AI brain for the desk companion bot.
    Maintains conversation history and calls the Anthropic API.
    """

    def __init__(self):
        self.face = None  # Set early so partial-init is safe
        if not ANTHROPIC_API_KEY or ANTHROPIC_API_KEY == "YOUR_KEY_HERE":
            raise ValueError(
                "No API key found. Set ANTHROPIC_API_KEY environment variable "
                "or update src/config.py"
            )
        self.client = Anthropic(api_key=ANTHROPIC_API_KEY)
        self.history = []  # Conversation history for multi-turn context
        self.face = Face(port=FACE_SERIAL_PORT, baud=FACE_SERIAL_BAUD)
        self.display = Display(
            spi_bus=DISPLAY_SPI_BUS,
            spi_device=DISPLAY_SPI_DEVICE,
            dc_pin=DISPLAY_DC_PIN,
            rst_pin=DISPLAY_RST_PIN,
        )
        self.voice = Voice(
            api_key=OPENAI_API_KEY,
            model=TTS_MODEL,
            voice=TTS_VOICE,
            playback_cmd=TTS_PLAYBACK_CMD,
        )
        log.info(f"Brain initialized. Model: {AI_MODEL}. Bot name: {BOT_NAME}. "
                 f"Face: {'connected' if self.face.is_connected() else 'offline'}. "
                 f"Display: {'connected' if self.display.is_connected() else 'offline'}. "
                 f"Voice: {'ready' if self.voice.is_available() else 'offline'}.")

        # Idle / dream tracking
        self.last_input_time = time.time()
        self._dream_stop = threading.Event()
        self._dream_thread = threading.Thread(target=self._dream_loop, daemon=True)
        self._dream_thread.start()

    def think(self, user_input: str) -> dict:
        """
        Send user input to the AI and return a structured response dict.
        This is the main method everything else calls.

        Returns: {emotion, speech, movement, led_color}
        """
        log.debug(f"User: {user_input}")
        self.last_input_time = time.time()

        # Add user message to history
        self.history.append({
            "role": "user",
            "content": user_input
        })

        try:
            response = self.client.messages.create(
                model=AI_MODEL,
                max_tokens=AI_MAX_TOKENS,
                system=SYSTEM_PROMPT,
                messages=self.history
            )

            raw_text = response.content[0].text
            log.debug(f"Raw API response: {raw_text}")

            parsed = parse_response(raw_text)

            # Add assistant response to history (use raw for context)
            self.history.append({
                "role": "assistant",
                "content": raw_text
            })

            # Keep history from growing too large (last 20 turns)
            if len(self.history) > 40:
                self.history = self.history[-40:]

            log.info(f"Emotion: {parsed['emotion']} | Speech: {parsed['speech']}")
            # Push emotion to the face controller
            color_hex = LED_COLORS_HEX.get(parsed["led_color"], LED_COLORS_HEX.get("warm_white", "#FFDCB4"))
            self.face.set_emotion(parsed["emotion"], color_hex)

            # Push head movement
            self.face.move(parsed["movement"])

            # Show the spoken text as a caption on the face
            self.face.say(parsed["speech"])

            # Mirror to the rectangular history display
            self.display.add_message("user", user_input)
            self.display.add_message("pip", parsed["speech"])
            self.display.update_status(parsed["emotion"], color_hex)

            # Speak the reply through the speaker
            self.voice.say(parsed["speech"])

            return parsed

        except Exception as e:
            log.error(f"API call failed: {e}")
            return DEFAULT_RESPONSE.copy()

    def _dream_loop(self):
        """Background thread: when Pip has been idle a while, generate dreamy
        captions periodically. Runs forever until process exit."""
        DREAM_IDLE_THRESHOLD_S = 60     # don't dream until idle this long
        DREAM_MIN_INTERVAL_S   = 25
        DREAM_MAX_INTERVAL_S   = 50

        while not self._dream_stop.is_set():
            # Wait a random amount of time
            wait_s = random.uniform(DREAM_MIN_INTERVAL_S, DREAM_MAX_INTERVAL_S)
            if self._dream_stop.wait(timeout=wait_s):
                return  # stop requested

            # Only dream if idle long enough
            idle_s = time.time() - self.last_input_time
            if idle_s < DREAM_IDLE_THRESHOLD_S:
                continue

            try:
                # 30% of the time, slip the current local time into the prompt
                # so Pip occasionally mumbles about it.
                include_time = random.random() < 0.30
                if include_time:
                    now_str = time.strftime("%-I:%M %p")  # e.g. "3:47 PM"
                    user_msg = f"(generate one dream mumble — the current time is {now_str}, work it in naturally and dreamily)"
                else:
                    user_msg = "(generate one dream mumble)"

                response = self.client.messages.create(
                    model=AI_MODEL,
                    max_tokens=60,
                    system=DREAM_PROMPT,
                    messages=[{"role": "user", "content": user_msg}],
                )
                dream_text = response.content[0].text.strip().strip('"').strip("'")
                if dream_text:
                    log.info(f"Pip dreams: {dream_text}")
                    if self.face is not None:
                        self.face.say(dream_text)
            except Exception as e:
                # Network blip, rate limit, whatever — log and keep going.
                log.warning(f"Dream generation failed: {e}")

    def clear_history(self):
        """Reset conversation context."""
        self.history = []
        log.info("Conversation history cleared.")


# ─── Stage 1: Terminal Chat Loop ───────────────────────────
def terminal_chat():
    """
    Simple terminal interface for Stage 1 testing.
    Tests the full brain pipeline with no hardware.
    Type 'quit' to exit, 'clear' to reset history.
    """
    print(f"\n{'='*50}")
    print(f"  {BOT_NAME} — Desk Companion Bot (Stage 1 Test)")
    print(f"  Model: {AI_MODEL}")
    print(f"  Type 'quit' to exit, 'clear' to reset history")
    print(f"{'='*50}\n")

    brain = Brain()
    try:
        # Greet on startup
        greeting = brain.think("Say hello and introduce yourself briefly.")
        print(f"\n🤖 [{greeting['emotion'].upper()}] {greeting['speech']}")
        print(f"   Movement: {greeting['movement']} | LED: {greeting['led_color']}\n")

        while True:
            try:
                user_input = read_line_with_live_face(brain.face, "You: ").strip()

                if not user_input:
                    continue
                if user_input.lower() == "quit":
                    print("Shutting down...")
                    break
                if user_input.lower() == "clear":
                    brain.clear_history()
                    print("(Conversation history cleared)\n")
                    continue

                response = brain.think(user_input)

                print(f"\n🤖 [{response['emotion'].upper()}] {response['speech']}")
                print(f"   Movement: {response['movement']} | LED: {response['led_color']}\n")

            except KeyboardInterrupt:
                print("\nShutting down...")
                break
    finally:
        # Stop dream thread before closing face
        if hasattr(brain, '_dream_stop'):
            brain._dream_stop.set()
        if hasattr(brain, 'voice') and brain.voice is not None:
            brain.voice.close()
        if brain.face is not None:
            brain.face.close()
        if hasattr(brain, 'display') and brain.display is not None:
            brain.display.close()


if __name__ == "__main__":
    terminal_chat()
