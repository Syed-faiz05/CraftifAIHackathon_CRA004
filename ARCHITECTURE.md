# AI Edge Traffic Intelligence: System Architecture

## Overview

The system processes prerecorded traffic video using NVIDIA DeepStream and exposes the generated analytics through a Flask dashboard.

## Processing Pipeline

```mermaid
flowchart TD
    A[Traffic Input Video] --> B[DeepStream Pipeline]
    B --> C[YOLO11s TensorRT FP16]
    C --> D[Vehicle Detection]
    D --> E[Object Tracking]
    E --> F[nvdsanalytics]
    F --> G[Line Crossing Counts]
    D --> H[Latest Frame Class Counts]
    G --> I[Annotated Video]
    H --> J[JSON Metrics]
    I --> K[Flask Dashboard]
    J --> K
    G --> L[CSV Report]
    H --> L
    L --> K

## Components

1. **Input:** Prerecorded traffic video.
2. **Detection:** YOLO11s TensorRT FP16 detects supported vehicle classes.
3. **Tracking:** DeepStream tracks detected objects across frames.
4. **Analytics:** `nvdsanalytics` counts crossings of the configured line.
5. **Outputs:** Annotated video, analytics logs, and latest-frame detection counts in JSON.
6. **Dashboard:** Flask serves the dashboard, video, JSON summary API, and CSV report.

## Data Semantics

- **Latest-frame detections:** Objects detected in the latest processed frame.
- **Cumulative line crossings:** Analytics count accumulated during processing.
- **Annotated video:** Recorded output containing analytics overlays.
- **CSV report:** Export of the available metrics.

These are different measurements and should not be interpreted interchangeably.

## Runtime Notes

- DeepStream dependencies run inside the configured Docker image.
- Flask runs in the Ubuntu host environment.
- The dashboard reads generated files; it does not independently run inference.
- Live camera integration and performance benchmarking remain future work.
