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
from anthropic import Anthropic

# ─── Setup ─────────────────────────────────────────────────
logging.basicConfig(level=logging.DEBUG)
log = logging.getLogger(__name__)

try:
    from config import (
        ANTHROPIC_API_KEY, AI_MODEL, AI_MAX_TOKENS,
        AI_TEMPERATURE, BOT_NAME
    )
except ImportError:
    # Fallback defaults if run standalone
    ANTHROPIC_API_KEY = os.environ.get("ANTHROPIC_API_KEY", "")
    AI_MODEL          = "claude-sonnet-4-5"
    AI_MAX_TOKENS     = 256
    AI_TEMPERATURE    = 0.8
    BOT_NAME          = "Pip"

# ─── System Prompt ─────────────────────────────────────────
SYSTEM_PROMPT = f"""You are {BOT_NAME}, a small friendly desktop robot companion.
You are witty, warm, curious, and occasionally playful. You live on the user's desk.
You have a physical body with a round screen face and you can move your head.

CRITICAL: Always respond ONLY with a single valid JSON object. No preamble, no markdown, no explanation.

Required format:
{{
  "emotion": "happy|thinking|sad|excited|neutral|surprised|sleepy",
  "speech": "Your spoken response here (1-2 sentences max)",
  "movement": "nod|shake|tilt_left|tilt_right|wiggle|idle",
  "led_color": "warm_white|blue|red|purple|green|yellow"
}}

Emotion guide:
- thinking: use while answering complex or thoughtful questions
- excited: good news, greetings, interesting topics
- happy: general positive, helpful responses
- sad: bad news, apologies, empathetic moments
- surprised: unexpected info, plot twists
- sleepy: late night, idle, low energy
- neutral: facts, short answers

Movement guide:
- nod: agreement, greeting, happy affirmation
- shake: disagreement, confusion
- tilt_left: thinking, curious
- tilt_right: surprised, playful
- wiggle: excited, dancing, celebration
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
    valid_emotions  = {"happy", "thinking", "sad", "excited", "neutral", "surprised", "sleepy"}
    valid_movements = {"nod", "shake", "tilt_left", "tilt_right", "wiggle", "idle"}
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
        if not ANTHROPIC_API_KEY or ANTHROPIC_API_KEY == "YOUR_KEY_HERE":
            raise ValueError(
                "No API key found. Set ANTHROPIC_API_KEY environment variable "
                "or update src/config.py"
            )
        self.client = Anthropic(api_key=ANTHROPIC_API_KEY)
        self.history = []  # Conversation history for multi-turn context
        log.info(f"Brain initialized. Model: {AI_MODEL}. Bot name: {BOT_NAME}")

    def think(self, user_input: str) -> dict:
        """
        Send user input to the AI and return a structured response dict.
        This is the main method everything else calls.

        Returns: {emotion, speech, movement, led_color}
        """
        log.debug(f"User: {user_input}")

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
            return parsed

        except Exception as e:
            log.error(f"API call failed: {e}")
            return DEFAULT_RESPONSE.copy()

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

    # Greet on startup
    greeting = brain.think("Say hello and introduce yourself briefly.")
    print(f"\n🤖 [{greeting['emotion'].upper()}] {greeting['speech']}")
    print(f"   Movement: {greeting['movement']} | LED: {greeting['led_color']}\n")

    while True:
        try:
            user_input = input("You: ").strip()

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


if __name__ == "__main__":
    terminal_chat()
