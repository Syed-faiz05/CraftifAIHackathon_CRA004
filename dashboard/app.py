from flask import Flask, jsonify, render_template, send_from_directory, Response
from pathlib import Path
from datetime import datetime, timezone
import csv
import io
import json
import re

BASE = Path("/home/goldie/traffic-data/portable")
OUT = BASE / "output"
APP = Flask(__name__)

def get_crossings():
    log = OUT / "analytics-restored-polyline.log"
    if not log.exists():
        return 0
    text = log.read_text(errors="ignore")
    matches = re.findall(
        r"LineCrossing Cumulative Exit\s*=\s*(\d+)", text
    )
    return int(matches[-1]) if matches else 0

def get_detections():
    path = OUT / "latest-detections.json"
    defaults = {
        "frame_number": 0,
        "car": 0,
        "motorcycle": 0,
        "bus": 0,
        "truck": 0,
    }
    try:
        data = json.loads(path.read_text())
        for key in defaults:
            value = data.get(key, defaults[key])
            defaults[key] = max(0, int(value))
        return defaults
    except (OSError, ValueError, TypeError, json.JSONDecodeError):
        return defaults

@APP.get("/")
def index():
    return render_template("index.html")

@APP.get("/api/summary")
def summary():
    detections = get_detections()
    json_exists = (OUT / "latest-detections.json").exists()
    return jsonify({
        "status": "Ready" if json_exists else "Waiting for detections",
        "motorcycle_crossings": get_crossings(),
        "detections": detections,
        "video": "/video",
        "source": "NVIDIA DeepStream"
    })

@APP.get("/video")
def video():
    return send_from_directory(OUT, "output-analytics-app.mp4")

@APP.get("/report.csv")
def report():
    detections = get_detections()
    data = io.StringIO()
    writer = csv.writer(data)
    writer.writerow([
        "generated_at_utc", "metric", "value", "unit", "source"
    ])
    now = datetime.now(timezone.utc).isoformat()

    writer.writerow([
        now, "motorcycle_line_crossings", get_crossings(),
        "crossings", "DeepStream nvdsanalytics log"
    ])
    writer.writerow([
        now, "video_resolution", "1920x1080",
        "pixels", "Input video metadata"
    ])

    for key, label in [
        ("car", "cars"),
        ("motorcycle", "motorcycles"),
        ("bus", "buses"),
        ("truck", "trucks"),
    ]:
        writer.writerow([
            now, f"latest_frame_{label}", detections[key],
            "detections", "DeepStream latest frame JSON"
        ])

    writer.writerow([
        now, "latest_frame_number", detections["frame_number"],
        "frame", "DeepStream latest frame JSON"
    ])

    return Response(
        data.getvalue(),
        mimetype="text/csv",
        headers={
            "Content-Disposition":
            "attachment; filename=traffic-report.csv"
        }
    )

if __name__ == "__main__":
    APP.run(host="127.0.0.1", port=5000, debug=False)
