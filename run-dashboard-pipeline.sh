#!/usr/bin/env bash
set -u

BASE="/home/goldie/traffic-data/portable"
IMAGE="craftifaidocker/deepstream:7.1-amd64-v3"
LOG="$BASE/output/dashboard-pipeline.log"

mkdir -p "$BASE/output"
exec 9>"$BASE/output/dashboard-pipeline.lock"

if ! flock -n 9; then
    echo "Pipeline already running."
    exit 1
fi

while true; do
    echo "[$(date -Is)] Starting analytics video pipeline." | tee -a "$LOG"

    docker run --rm --runtime=nvidia --network=host \
        -v /home/goldie/traffic-data:/home/goldie/traffic-data \
        -v "$BASE:/workspace" \
        --entrypoint /bin/bash "$IMAGE" -c '
          set -e
          export LD_LIBRARY_PATH="/opt/nvidia/deepstream/deepstream-7.1/lib:/usr/local/cuda-12.6/lib64:${LD_LIBRARY_PATH:-}"
          cd /workspace
          bash ./run-analytics.sh
        ' >>"$LOG" 2>&1
    VIDEO_STATUS=$?

    if [ "$VIDEO_STATUS" -eq 0 ]; then
        echo "[$(date -Is)] Analytics video completed." | tee -a "$LOG"
    else
        echo "[$(date -Is)] Analytics video failed with exit code $VIDEO_STATUS." | tee -a "$LOG"
    fi

    echo "[$(date -Is)] Starting JSON detection pipeline." | tee -a "$LOG"

    docker run --rm --runtime=nvidia --network=host \
        -v /home/goldie/traffic-data:/home/goldie/traffic-data \
        -v "$BASE:/workspace" \
        --entrypoint /bin/bash "$IMAGE" -c '
          set -e
          export LD_LIBRARY_PATH="/opt/nvidia/deepstream/deepstream-7.1/lib:/usr/local/cuda-12.6/lib64:${LD_LIBRARY_PATH:-}"
          exec /workspace/analytics-overlay-source/build-vehicle-counts/deepstream-analytics-vehicle-json-test \
            -c /workspace/configs/traffic-analytics-app.txt
        ' >>"$LOG" 2>&1
    JSON_STATUS=$?

    if [ "$JSON_STATUS" -eq 0 ]; then
        echo "[$(date -Is)] JSON detections updated." | tee -a "$LOG"
    else
        echo "[$(date -Is)] JSON pipeline failed with exit code $JSON_STATUS." | tee -a "$LOG"
    fi

    sleep 5
done
