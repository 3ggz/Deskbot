#!/bin/bash
# scripts/setup.sh
# Run this on the Raspberry Pi after first boot to install everything.
# Usage: bash scripts/setup.sh

set -e  # Exit on any error

echo "=========================================="
echo "  Desk Companion Bot — Pi Setup Script"
echo "=========================================="

# ─── System Updates ────────────────────────────────────────
echo "[1/6] Updating system packages..."
sudo apt-get update -qq
sudo apt-get upgrade -y -qq

# ─── System Dependencies ───────────────────────────────────
echo "[2/6] Installing system dependencies..."
sudo apt-get install -y -qq \
    python3-pip \
    python3-venv \
    python3-dev \
    python3-smbus \
    i2c-tools \
    portaudio19-dev \
    libportaudio2 \
    libportaudiocpp0 \
    ffmpeg \
    git \
    libgpiod2

# ─── Enable Pi Interfaces ──────────────────────────────────
echo "[3/6] Enabling SPI and I2C..."
sudo raspi-config nonint do_spi 0      # Enable SPI
sudo raspi-config nonint do_i2c 0     # Enable I2C

# Enable I2S audio (for INMP441 mic and PCM5102 DAC)
if ! grep -q "dtoverlay=i2s-mmap" /boot/config.txt; then
    echo "dtoverlay=i2s-mmap" | sudo tee -a /boot/config.txt
fi

# ─── Python Virtual Environment ────────────────────────────
echo "[4/6] Creating Python virtual environment..."
python3 -m venv venv
source venv/bin/activate

# ─── Python Packages ───────────────────────────────────────
echo "[5/6] Installing Python packages..."
pip install --upgrade pip -q
pip install -r requirements.txt -q

# ─── Environment Setup ─────────────────────────────────────
echo "[6/6] Setting up environment..."

if [ ! -f ".env" ]; then
    cat > .env << EOF
# Desk Companion Bot — Environment Variables
# Fill in your actual API keys below
ANTHROPIC_API_KEY=your_anthropic_api_key_here

# OpenAI key — used for TTS (Pip's voice) and Whisper STT (future)
OPENAI_API_KEY=your_openai_api_key_here
EOF
    echo "  Created .env file — add your API keys!"
fi

# Add .env to .gitignore
if [ ! -f ".gitignore" ]; then
    cat > .gitignore << EOF
.env
venv/
__pycache__/
*.pyc
*.pyo
.DS_Store
EOF
fi

echo ""
echo "=========================================="
echo "  Setup complete!"
echo ""
echo "  Next steps:"
echo "  1. Edit .env and add your ANTHROPIC_API_KEY"
echo "  2. Reboot for I2S changes to take effect:"
echo "       sudo reboot"
echo "  3. After reboot, activate venv and run Stage 1:"
echo "       source venv/bin/activate"
echo "       cd src && python3 brain.py"
echo ""
echo "  To verify I2C (after wiring PCA9685):"
echo "       sudo i2cdetect -y 1"
echo "=========================================="
