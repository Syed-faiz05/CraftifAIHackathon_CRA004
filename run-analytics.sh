#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TRACKER_LIB="/opt/nvidia/deepstream/deepstream-7.1/lib/libnvds_nvmultiobjecttracker.so"
TRACKER_CONFIG="$DIR/configs/config_tracker_IOU.yml"
ANALYTICS_CONFIG="$DIR/configs/config_nvdsanalytics.txt"
ENGINE="$DIR/yolo11s.fp16.plan"
INPUT="/home/goldie/traffic-data/traffic.mp4"
OUTPUT="$DIR/output/output-analytics.mp4"

mkdir -p "$DIR/output"

for f in "$TRACKER_LIB" "$TRACKER_CONFIG" "$ANALYTICS_CONFIG" \
         "$ENGINE" "$INPUT" "$DIR/configs/config_det_pgie.txt"; do
    if [ ! -f "$f" ]; then
        echo "Missing required file: $f" >&2
        exit 1
    fi
done

echo "Starting YOLO11s + IOU tracking + line-crossing analytics..."

gst-launch-1.0 -e \
  nvstreammux name=mux batch-size=1 width=1920 height=1080 \
    batched-push-timeout=40000 live-source=0 \
  nvurisrcbin uri="file://$INPUT" ! mux.sink_0 \
  mux. ! queue max-size-buffers=4 \
  ! nvvideoconvert \
  ! queue max-size-buffers=4 \
  ! nvinfer_backport config-file-path="$DIR/configs/config_det_pgie.txt" \
  ! queue max-size-buffers=4 \
  ! nvtracker \
      ll-lib-file="$TRACKER_LIB" \
      ll-config-file="$TRACKER_CONFIG" \
      tracker-width=960 \
      tracker-height=544 \
      display-tracking-id=true \
  ! queue max-size-buffers=4 \
  ! nvdsanalytics config-file="$ANALYTICS_CONFIG" \
  ! queue max-size-buffers=4 \
  ! nvvideoconvert \
  ! nvdsosd \
  ! queue max-size-buffers=4 \
  ! nvvideoconvert \
  ! 'video/x-raw(memory:NVMM),format=NV12' \
  ! nvvideoconvert \
  ! 'video/x-raw,format=I420' \
  ! videoconvert \
  ! x264enc speed-preset=veryfast tune=zerolatency bitrate=8000 key-int-max=30 \
  ! h264parse \
  ! qtmux \
  ! filesink location="$OUTPUT"

echo "Analytics pipeline finished."
echo "Output: $OUTPUT"
