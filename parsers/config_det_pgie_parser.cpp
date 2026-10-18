// Custom detection (bbox) parser (generated).
// Scaffold validates the output layer and exposes the raw tensor(s); the DECODE
// block (per-model) populates `objectList` with NvDsInferObjectDetectionInfo.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "nvdsinfer_custom_impl.h"

extern "C" bool ParseConfigDetPgie(
    std::vector<NvDsInferLayerInfo> const& outputLayersInfo,
    NvDsInferNetworkInfo const& networkInfo,
    NvDsInferParseDetectionParams const& detectionParams,
    std::vector<NvDsInferObjectDetectionInfo>& objectList);

CHECK_CUSTOM_PARSE_FUNC_PROTOTYPE(ParseConfigDetPgie);

extern "C" bool ParseConfigDetPgie(
    std::vector<NvDsInferLayerInfo> const& outputLayersInfo,
    NvDsInferNetworkInfo const& networkInfo,
    NvDsInferParseDetectionParams const& detectionParams,
    std::vector<NvDsInferObjectDetectionInfo>& objectList)
{
    if (outputLayersInfo.empty()) return false;
    auto const& layer0 = outputLayersInfo[0];

    const unsigned numClasses = detectionParams.numClassesConfigured;
    const float netW = static_cast<float>(networkInfo.width);
    const float netH = static_cast<float>(networkInfo.height);
    const float* data = static_cast<const float*>(layer0.buffer);

    static bool logged = false;
    if (!logged) {
        logged = true;
        fprintf(stderr, "[YOLO11sDet] first call: layer=%s numDims=%d "
                "networkInput=%.0fx%.0f numClasses=%u outputs=%zu\n",
                layer0.layerName ? layer0.layerName : "?",
                layer0.inferDims.numDims, netW, netH, numClasses,
                outputLayersInfo.size());
    }

    // ==== BEGIN GENERATED DECODE ====
    // Output tensor shape (batch stripped): [84, 8400]
    // inferDims.numDims == 2, d[0] == 84, d[1] == 8400
    // OR d[0] == num_features, d[1] == num_proposals
    // We need to figure out which dim is feature (==4+numClasses) and which is count.

    const int dim0 = static_cast<int>(layer0.inferDims.d[0]);
    const int dim1 = static_cast<int>(layer0.inferDims.d[1]);

    int numFeatures, numProposals;
    bool transposed; // true if layout is [features, proposals] (need transpose access)

    const int expectedFeatures = 4 + static_cast<int>(numClasses);

    if (dim0 == expectedFeatures) {
        // [features, proposals] — standard YOLO11/YOLOv8 output
        numFeatures = dim0;
        numProposals = dim1;
        transposed = true;
    } else if (dim1 == expectedFeatures) {
        // [proposals, features]
        numFeatures = dim1;
        numProposals = dim0;
        transposed = false;
    } else {
        fprintf(stderr, "[YOLO11sDet] ERROR: cannot match dims (%d,%d) to "
                "expected feature count %d\n", dim0, dim1, expectedFeatures);
        return false;
    }

    // Allowed road-user COCO class IDs
    static const int allowedClasses[] = {0, 1, 2, 3, 5, 7};
    static const int numAllowed = 6;

    // NMS IoU threshold
    const float nmsIouThreshold = 0.45f;

    // Temporary storage for NMS
    struct RawDet {
        float x1, y1, x2, y2, score;
        unsigned classId;
    };

    // Per-class buckets
    std::vector<std::vector<RawDet>> classBuckets(numClasses);

    for (int i = 0; i < numProposals; ++i) {
        // Access cx, cy, w, h
        float cx, cy, bw, bh;
        if (transposed) {
            // data layout: data[feature * numProposals + proposal]
            cx = data[0 * numProposals + i];
            cy = data[1 * numProposals + i];
            bw = data[2 * numProposals + i];
            bh = data[3 * numProposals + i];
        } else {
            cx = data[i * numFeatures + 0];
            cy = data[i * numFeatures + 1];
            bw = data[i * numFeatures + 2];
            bh = data[i * numFeatures + 3];
        }

        // Find best class
        float maxScore = -1.0f;
        unsigned bestClass = 0;
        for (unsigned c = 0; c < numClasses; ++c) {
            float s;
            if (transposed) {
                s = data[(4 + c) * numProposals + i];
            } else {
                s = data[i * numFeatures + 4 + c];
            }
            if (s > maxScore) {
                maxScore = s;
                bestClass = c;
            }
        }

        // Threshold check
        float thr = (bestClass < detectionParams.perClassPreclusterThreshold.size())
                     ? detectionParams.perClassPreclusterThreshold[bestClass]
                     : 0.3f;
        if (maxScore < thr) continue;

        // Check if this class is an allowed road-user class
        bool allowed = false;
        for (int a = 0; a < numAllowed; ++a) {
            if (static_cast<int>(bestClass) == allowedClasses[a]) {
                allowed = true;
                break;
            }
        }
        if (!allowed) continue;

        // cx/cy/w/h are in network-input pixel coords (YOLO11 export)
        float x1 = cx - bw * 0.5f;
        float y1 = cy - bh * 0.5f;
        float x2 = cx + bw * 0.5f;
        float y2 = cy + bh * 0.5f;

        // Clamp to network input
        x1 = std::max<float>(0.0f, std::min<float>(x1, netW));
        y1 = std::max<float>(0.0f, std::min<float>(y1, netH));
        x2 = std::max<float>(0.0f, std::min<float>(x2, netW));
        y2 = std::max<float>(0.0f, std::min<float>(y2, netH));

        // Skip degenerate
        if ((x2 - x1) <= 1.0f || (y2 - y1) <= 1.0f) continue;

        RawDet rd;
        rd.x1 = x1;
        rd.y1 = y1;
        rd.x2 = x2;
        rd.y2 = y2;
        rd.score = maxScore;
        rd.classId = bestClass;
        classBuckets[bestClass].push_back(rd);
    }

    // Per-class NMS
    for (unsigned c = 0; c < numClasses; ++c) {
        auto& dets = classBuckets[c];
        if (dets.empty()) continue;

        // Sort by score descending
        std::sort(dets.begin(), dets.end(),
                  [](const RawDet& a, const RawDet& b) {
                      return a.score > b.score;
                  });

        std::vector<bool> suppressed(dets.size(), false);
        for (size_t i = 0; i < dets.size(); ++i) {
            if (suppressed[i]) continue;

            NvDsInferObjectDetectionInfo obj;
            obj.classId = dets[i].classId;
            obj.left = dets[i].x1;
            obj.top = dets[i].y1;
            obj.width = dets[i].x2 - dets[i].x1;
            obj.height = dets[i].y2 - dets[i].y1;
            obj.detectionConfidence = dets[i].score;
            objectList.push_back(obj);

            // Suppress overlapping boxes
            float area_i = obj.width * obj.height;
            for (size_t j = i + 1; j < dets.size(); ++j) {
                if (suppressed[j]) continue;

                float ix1 = std::max<float>(dets[i].x1, dets[j].x1);
                float iy1 = std::max<float>(dets[i].y1, dets[j].y1);
                float ix2 = std::min<float>(dets[i].x2, dets[j].x2);
                float iy2 = std::min<float>(dets[i].y2, dets[j].y2);
                float iw = std::max<float>(0.0f, ix2 - ix1);
                float ih = std::max<float>(0.0f, iy2 - iy1);
                float inter = iw * ih;
                float area_j = (dets[j].x2 - dets[j].x1) * (dets[j].y2 - dets[j].y1);
                float unionArea = area_i + area_j - inter;
                if (unionArea > 0.0f && (inter / unionArea) > nmsIouThreshold) {
                    suppressed[j] = true;
                }
            }
        }
    }
    // ==== END GENERATED DECODE ====
    return true;
}