#pragma once

namespace openrhi {

static inline bool GetLatencyMarkerVK(LatencyMarker marker, uint32_t& nativeMarker) {
    if (marker >= LatencyMarker::MAX_NUM || marker == LatencyMarker::PC_LATENCY_PING)
        return false;

    nativeMarker = (uint32_t)marker;

    if (marker >= LatencyMarker::OUT_OF_BAND_RENDER_SUBMIT_START)
        --nativeMarker;

    return true;
}

} // namespace openrhi
