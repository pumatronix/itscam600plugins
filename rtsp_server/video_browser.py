#!/usr/bin/env python3
"""
Video Browser Web Application
Provides a web interface to browse and query video files by date and time.
"""

import os
import re
from datetime import datetime, timedelta
from flask import Flask, render_template, request, jsonify, send_file, abort
from pathlib import Path
import mimetypes

app = Flask(__name__)

# Configuration
VIDEO_DIR = "/videos"
ALLOWED_EXTENSIONS = {'.mp4', '.avi', '.mkv', '.mov', '.wmv', '.flv', '.webm'}

class VideoFile:
    def __init__(self, filepath):
        self.filepath = filepath
        self.filename = os.path.basename(filepath)
        self.size = os.path.getsize(filepath)
        self.modified_time = datetime.fromtimestamp(os.path.getmtime(filepath))
        self.created_time = datetime.fromtimestamp(os.path.getctime(filepath))

        # Parse date/time from filename if it follows the pattern: stream_YYYY-MM-DD_HH-MM-SS.mp4
        self.parsed_datetime = self._parse_datetime_from_filename()

    def _parse_datetime_from_filename(self):
        """Extract datetime from filename pattern: stream_YYYY-MM-DD_HH-MM-SS.mp4"""
        pattern = r'stream_(\d{4})-(\d{2})-(\d{2})_(\d{2})-(\d{2})-(\d{2})\.mp4'
        match = re.match(pattern, self.filename)
        if match:
            year, month, day, hour, minute, second = map(int, match.groups())
            return datetime(year, month, day, hour, minute, second)
        return None
    
    def format_size(self):
        """Format file size in human readable format"""
        for unit in ['B', 'KB', 'MB', 'GB']:
            if self.size < 1024.0:
                return f"{self.size:.1f} {unit}"
            self.size /= 1024.0
        return f"{self.size:.1f} TB"
    
    def to_dict(self):
        return {
            'filename': self.filename,
            'filepath': self.filepath,
            'size': self.format_size(),
            'size_bytes': os.path.getsize(self.filepath),
            'modified_time': self.modified_time.isoformat(),
            'created_time': self.created_time.isoformat(),
            'parsed_datetime': self.parsed_datetime.isoformat() if self.parsed_datetime else None,
            'display_datetime': self.parsed_datetime.strftime('%Y-%m-%d %H:%M:%S') if self.parsed_datetime else self.modified_time.strftime('%Y-%m-%d %H:%M:%S')
        }

def get_video_files():
    """Get all video files from the video directory"""
    if not os.path.exists(VIDEO_DIR):
        return []
    
    video_files = []
    for filename in os.listdir(VIDEO_DIR):
        filepath = os.path.join(VIDEO_DIR, filename)
        if os.path.isfile(filepath) and Path(filename).suffix.lower() in ALLOWED_EXTENSIONS:
            video_files.append(VideoFile(filepath))
    
    # Sort by parsed datetime if available, otherwise by modified time
    video_files.sort(key=lambda x: x.parsed_datetime or x.modified_time, reverse=True)
    return video_files

def filter_videos_by_date_range(video_files, start_date=None, end_date=None, start_time=None, end_time=None):
    """Filter video files by date and time range"""
    filtered_videos = []
    
    for video in video_files:
        # Use parsed datetime if available, otherwise use modified time
        video_datetime = video.parsed_datetime or video.modified_time
        
        # Check date range
        if start_date:
            start_datetime = datetime.strptime(start_date, '%Y-%m-%d')
            if video_datetime.date() < start_datetime.date():
                continue
        
        if end_date:
            end_datetime = datetime.strptime(end_date, '%Y-%m-%d')
            if video_datetime.date() > end_datetime.date():
                continue
        
        # Check time range (only if both start and end times are provided)
        if start_time and end_time:
            video_time = video_datetime.time()
            start_time_obj = datetime.strptime(start_time, '%H:%M').time()
            end_time_obj = datetime.strptime(end_time, '%H:%M').time()
            
            if not (start_time_obj <= video_time <= end_time_obj):
                continue
        
        filtered_videos.append(video)
    
    return filtered_videos

@app.route('/')
def index():
    """Main page with video browser"""
    return render_template('index.html')

@app.route('/api/videos')
def api_videos():
    """API endpoint to get video files with optional filtering"""
    try:
        start_date = request.args.get('start_date')
        end_date = request.args.get('end_date')
        start_time = request.args.get('start_time')
        end_time = request.args.get('end_time')
        
        video_files = get_video_files()
        
        if start_date or end_date or start_time or end_time:
            video_files = filter_videos_by_date_range(
                video_files, start_date, end_date, start_time, end_time
            )
        
        return jsonify({
            'success': True,
            'videos': [video.to_dict() for video in video_files],
            'total': len(video_files)
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/video/<path:filename>')
def serve_video(filename):
    """Serve video files"""
    try:
        video_path = os.path.join(VIDEO_DIR, filename)
        if not os.path.exists(video_path):
            abort(404)
        
        # Security check - ensure the file is within the video directory
        if not os.path.abspath(video_path).startswith(os.path.abspath(VIDEO_DIR)):
            abort(403)
        
        return send_file(video_path)
    except Exception as e:
        abort(500)

@app.route('/download/<path:filename>')
def download_video(filename):
    """Download video files"""
    try:
        video_path = os.path.join(VIDEO_DIR, filename)
        if not os.path.exists(video_path):
            abort(404)
        
        # Security check - ensure the file is within the video directory
        if not os.path.abspath(video_path).startswith(os.path.abspath(VIDEO_DIR)):
            abort(403)
        
        return send_file(video_path, as_attachment=True)
    except Exception as e:
        abort(500)

@app.route('/api/stats')
def api_stats():
    """Get statistics about video files"""
    try:
        video_files = get_video_files()
        
        if not video_files:
            return jsonify({
                'success': True,
                'total_files': 0,
                'total_size': '0 B',
                'date_range': None
            })
        
        total_size = sum(os.path.getsize(video.filepath) for video in video_files)
        
        # Format total size
        size = total_size
        for unit in ['B', 'KB', 'MB', 'GB']:
            if size < 1024.0:
                total_size_formatted = f"{size:.1f} {unit}"
                break
            size /= 1024.0
        else:
            total_size_formatted = f"{size:.1f} TB"
        
        # Get date range
        dates = [video.parsed_datetime or video.modified_time for video in video_files]
        date_range = {
            'earliest': min(dates).strftime('%Y-%m-%d %H:%M:%S'),
            'latest': max(dates).strftime('%Y-%m-%d %H:%M:%S')
        }
        
        return jsonify({
            'success': True,
            'total_files': len(video_files),
            'total_size': total_size_formatted,
            'date_range': date_range
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

if __name__ == '__main__':
    # Create videos directory if it doesn't exist
    os.makedirs(VIDEO_DIR, exist_ok=True)
    
    # Run the application
    app.run(host='0.0.0.0', port=9080, debug=True)
