#!/bin/bash

OUTPUT_DIR="/videos"
RESTART_DELAY=5
FILE_SIZE_MB=30  # Average file size in MB

# Calculate max files based on 80% of available disk space
calculate_max_files() {
  # Get available disk space in KB
  AVAILABLE_KB=$(df "$OUTPUT_DIR" | tail -1 | awk '{print $4}')
  # Convert to MB
  AVAILABLE_MB=$((AVAILABLE_KB / 1024))
  # Calculate 80% of available space
  USABLE_MB=$((AVAILABLE_MB * 80 / 100))
  # Calculate max files (minimum 10 files)
  MAX_FILES=$((USABLE_MB / FILE_SIZE_MB))
  if [ "$MAX_FILES" -lt 10 ]; then
    MAX_FILES=10
  fi
  echo "[$(date)] Available space: ${AVAILABLE_MB}MB, Max files: $MAX_FILES (using 80% = ${USABLE_MB}MB)"
}

cleanup_old_files() {
  # Recalculate max files based on current disk space
  calculate_max_files
  
  FILE_COUNT=$(ls "$OUTPUT_DIR"/stream_*.mp4 2>/dev/null | wc -l)
  if [ "$FILE_COUNT" -gt "$MAX_FILES" ]; then
    FILES_TO_DELETE=$((FILE_COUNT - MAX_FILES))
    echo "[$(date)] Cleaning up $FILES_TO_DELETE old files (current: $FILE_COUNT, max: $MAX_FILES)"
    ls -1t "$OUTPUT_DIR"/stream_*.mp4 | tail -n +$((MAX_FILES + 1)) | xargs rm -f
  fi
}

start_ffmpeg() {
  echo "[$(date)] Starting ffmpeg..."
  ffmpeg -rtsp_transport tcp -i rtsp://localhost:8554/stream \
    -c copy -f segment -strftime 1 -segment_time 600 -reset_timestamps 1 \
    "$OUTPUT_DIR/stream_%Y-%m-%d_%H-%M-%S.mp4" &
  return $!
}

start_web_server() {
  echo "[$(date)] Starting video browser web server..."
  cd /usr/local/bin
  python3 video_browser.py &
  return $!
}

# Start web server
start_web_server
WEB_SERVER_PID=$!
echo "[$(date)] Web server started with PID $WEB_SERVER_PID"

# Wait a moment for MediaMTX to be available before starting ffmpeg
echo "[$(date)] Waiting for MediaMTX to be available..."
sleep 10

# Main loop to keep ffmpeg running
while true; do
  start_ffmpeg
  FFMPEG_PID=$!
  echo "[$(date)] FFmpeg started with PID $FFMPEG_PID"
  
  # Monitor ffmpeg and clean up files
  while kill -0 $FFMPEG_PID 2>/dev/null; do
    sleep 60
    cleanup_old_files
    
    # Check if web server is still running, restart if needed
    if ! kill -0 $WEB_SERVER_PID 2>/dev/null; then
      echo "[$(date)] Web server process died, restarting..."
      start_web_server
      WEB_SERVER_PID=$!
      echo "[$(date)] Web server restarted with PID $WEB_SERVER_PID"
    fi
  done
  
  echo "[$(date)] FFmpeg process (PID $FFMPEG_PID) has exited. Restarting in $RESTART_DELAY seconds..."
  sleep $RESTART_DELAY
done
