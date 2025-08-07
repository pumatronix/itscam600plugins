# RTSP Server with Video Recording and Web Browser

This plugin implements an RTSP server with automatic video recording and a web-based video browser interface.

## Features

- **RTSP Streaming**: MediaMTX-based RTSP server on port 8554
- **Automatic Recording**: Continuous video recording in 10-minute segments
- **File Management**: Automatic cleanup of old files (keeps last 50 files)
- **Web Interface**: Browser-based video viewer and manager on port 9080
- **Search & Filter**: Query videos by date and time ranges
- **Download Support**: Direct download of video files
- **Process Monitoring**: Automatic restart of failed processes

## Services

### RTSP Server (Port 8554)
- Stream endpoint: `rtsp://localhost:8554/stream`
- Powered by MediaMTX
- Supports TCP transport

### Web Interface (Port 9080)
- Video browser with thumbnail previews
- Date and time filtering
- File statistics and management
- Download functionality
- Responsive design for mobile and desktop

## File Organization

Videos are automatically saved in the `/videos` directory with the naming pattern:
```
stream_YYYY-MM-DD_HH-MM-SS.mp4
```

## Build

This plugin uses Docker to create a complete container with all services. Build using:

```bash
./build.sh
```

## Usage

1. **Start the container** with the RTSP server plugin
2. **Stream to RTSP**: Send video to `rtsp://container_ip:8554/stream`
3. **Access Web Interface**: Navigate to `http://container_ip:9080`
4. **Browse Videos**: Use the web interface to view, search, and download recordings

## Web Interface Features

- **Dashboard**: Overview of total videos, storage usage, and date range
- **Search Filters**: Filter by start/end date and time range
- **Video Grid**: Thumbnail view of all recordings
- **Inline Preview**: Click play overlay to preview videos
- **Full Screen**: Click "Watch" to open video in new tab
- **Download**: Direct download of video files

## Configuration

- **Max Files**: Automatically keeps the last 50 video files (configurable in entrypoint.sh)
- **Segment Duration**: 10-minute video segments (600 seconds)
- **Video Directory**: `/videos` (mounted as volume)
- **Web Port**: 9080
- **RTSP Port**: 8554