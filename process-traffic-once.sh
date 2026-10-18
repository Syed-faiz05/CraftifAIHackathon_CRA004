#!/usr/bin/env bash
set -euo pipefail

BASE="/home/goldie/traffic-data/portable"
IMAGE="craftifaidocker/deepstream:7.1-amd64-v3"

mkdir -p "$BASE/output"

echo "Starting traffic analytics..."

docker run --rm --runtime=nvidia \
  -v /home/goldie/traffic-data:/home/goldie/traffic-data \
  -v "$BASE:/workspace" \
  --entrypoint /bin/bash "$IMAGE" -c '
    set -e
    export LD_LIBRARY_PATH="/opt/nvidia/deepstream/deepstream-7.1/lib:/usr/local/cuda-12.6/lib64:${LD_LIBRARY_PATH:-}"
    cd /workspace
    bash ./run-analytics.sh
  '

echo "Exporting latest detection metrics..."

docker run --rm --runtime=nvidia \
  -v /home/goldie/traffic-data:/home/goldie/traffic-data \
  -v "$BASE:/workspace" \
  --entrypoint /bin/bash "$IMAGE" -c '
    set -e
    export LD_LIBRARY_PATH="/opt/nvidia/deepstream/deepstream-7.1/lib:/usr/local/cuda-12.6/lib64:${LD_LIBRARY_PATH:-}"
    exec /workspace/analytics-overlay-source/build-vehicle-counts/deepstream-analytics-vehicle-json-test \
      -c /workspace/configs/traffic-analytics-app.txt
  '

echo
echo "Processing complete."
echo "Detection metrics:"
cat "$BASE/output/latest-detections.json"
echo
echo "Dashboard: http://127.0.0.1:5000"
