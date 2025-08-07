# Using a Media MTX image for ARM64 (multiplataforma)
FROM --platform=linux/arm64 bluenviron/mediamtx:latest-ffmpeg

# Install Python and dependencies for the web server
RUN apk add --no-cache python3 py3-pip python3-dev py3-flask

# Copy a custom yaml file to configure the mediamtx
COPY mediamtx.yml /mediamtx.yml

# Copy web server files
COPY video_browser.py /usr/local/bin/video_browser.py
COPY templates/ /usr/local/bin/templates/

# Copy and set up entrypoint script
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

# Create a volume to save videos
VOLUME /videos

# Expose ports for RTSP server (8554) and web interface (9080)
EXPOSE 8554 9080


