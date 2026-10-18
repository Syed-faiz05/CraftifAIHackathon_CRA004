# AI Edge Traffic Intelligence

An NVIDIA DeepStream-based traffic analytics prototype for prerecorded traffic video.

## Features
- YOLO11s TensorRT FP16 vehicle detection
- Object tracking and motorcycle line-crossing analytics
- Annotated video with detection and crossing overlays
- Flask dashboard
- JSON metrics and CSV report export

## Technology Stack
- NVIDIA DeepStream 7.1
- TensorRT and GStreamer
- YOLO11s
- nvdsanalytics
- Python and Flask
- Docker with NVIDIA GPU support
- Ubuntu 22.04 on WSL2

## Start the Dashboard
Open an Ubuntu terminal and run:

    cd /home/goldie/traffic-data/portable/dashboard
    .venv/bin/python app.py

Then open http://127.0.0.1:5000 in your browser.

## Dashboard Endpoints
- `/` - Dashboard
- `/api/summary` - Current metrics in JSON
- `/video` - Annotated video
- `/report.csv` - CSV report

## Process the Test Video
From the project directory:

    cd /home/goldie/traffic-data/portable
    ./process-traffic-once.sh

This uses GPU resources while processing the prerecorded video.

## Metric Definitions
Latest-frame detection counts describe one processed frame. The cumulative motorcycle line-crossing count is a separate analytics metric.

## Current Limitations
- Uses prerecorded video; live camera support is not yet verified.
- FPS, latency, and detection accuracy have not yet been benchmarked.
- TensorRT engines and custom parsers may require rebuilding on other hardware.

## Current Test Results
The validated sample run reported 5 cars and 2 motorcycles in its latest frame, with 3 motorcycle line crossings. These are sample-run results, not guaranteed results for other videos.

## Actual PipeGen Pipeline IR Graph


See [PIPELINE_IR_GRAPH.md](pipegen-project/PIPELINE_IR_GRAPH.md) for the graph generated from `PipelineIR.json`.
