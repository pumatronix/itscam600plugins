#!/bin/bash

# Video Browser Web Server Startup Script

echo "Starting Video Browser Web Server..."

# Install dependencies if needed
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
fi

echo "Activating virtual environment..."
source venv/bin/activate

echo "Installing/updating dependencies..."
pip install -r requirements_web.txt

echo "Starting web server on http://0.0.0.0:8080"
python3 video_browser.py
