#include <d3d12.h>
#include <dxgi.h>
#include <vulkan/vulkan_core.h>
#include <cstdio>
#include "OpenRHI.h"

#include "Extensions/OpenRHILowLatency.h"
#include "Latency.h"
#include "nvapi.h"

using namespace openrhi;

static_assert((uint32_t)LatencyMarker::SIMULATION_START == SIMULATION_START);
static_assert((uint32_t)LatencyMarker::SIMULATION_END == SIMULATION_END);
static_assert((uint32_t)LatencyMarker::RENDER_SUBMIT_START == RENDERSUBMIT_START);
static_assert((uint32_t)LatencyMarker::RENDER_SUBMIT_END == RENDERSUBMIT_END);
static_assert((uint32_t)LatencyMarker::INPUT_SAMPLE == INPUT_SAMPLE);
static_assert((uint32_t)LatencyMarker::TRIGGER_FLASH == TRIGGER_FLASH);
static_assert((uint32_t)LatencyMarker::PC_LATENCY_PING == PC_LATENCY_PING);
static_assert((uint32_t)LatencyMarker::OUT_OF_BAND_RENDER_SUBMIT_START == OUT_OF_BAND_RENDERSUBMIT_START);
static_assert((uint32_t)LatencyMarker::OUT_OF_BAND_RENDER_SUBMIT_END == OUT_OF_BAND_RENDERSUBMIT_END);
static_assert((uint32_t)LatencyMarker::OUT_OF_BAND_PRESENT_START == OUT_OF_BAND_PRESENT_START);
static_assert((uint32_t)LatencyMarker::OUT_OF_BAND_PRESENT_END == OUT_OF_BAND_PRESENT_END);
static_assert((uint32_t)LatencyQueueRole::RENDER == OUT_OF_BAND_RENDER);
static_assert((uint32_t)LatencyQueueRole::PRESENT == OUT_OF_BAND_PRESENT);
static_assert((uint32_t)LatencyQueueRole::IGNORED == OUT_OF_BAND_IGNORE);
static_assert((uint32_t)LatencyQueueRole::RENDER_PRESENT == OUT_OF_BAND_RENDER_PRESENT);
static_assert((uint32_t)LatencyQueueRole::EXPLICIT_COPY == OUT_OF_BAND_EXPLICIT_COPY);

int main() {
    const LatencyMarker markers[] = {LatencyMarker::SIMULATION_START, LatencyMarker::SIMULATION_END,
        LatencyMarker::RENDER_SUBMIT_START, LatencyMarker::RENDER_SUBMIT_END, LatencyMarker::INPUT_SAMPLE,
        LatencyMarker::TRIGGER_FLASH, LatencyMarker::OUT_OF_BAND_RENDER_SUBMIT_START,
        LatencyMarker::OUT_OF_BAND_RENDER_SUBMIT_END, LatencyMarker::OUT_OF_BAND_PRESENT_START, LatencyMarker::OUT_OF_BAND_PRESENT_END};
    const VkLatencyMarkerNV native[] = {VK_LATENCY_MARKER_SIMULATION_START_NV, VK_LATENCY_MARKER_SIMULATION_END_NV,
        VK_LATENCY_MARKER_RENDERSUBMIT_START_NV, VK_LATENCY_MARKER_RENDERSUBMIT_END_NV, VK_LATENCY_MARKER_INPUT_SAMPLE_NV,
        VK_LATENCY_MARKER_TRIGGER_FLASH_NV, VK_LATENCY_MARKER_OUT_OF_BAND_RENDERSUBMIT_START_NV,
        VK_LATENCY_MARKER_OUT_OF_BAND_RENDERSUBMIT_END_NV, VK_LATENCY_MARKER_OUT_OF_BAND_PRESENT_START_NV, VK_LATENCY_MARKER_OUT_OF_BAND_PRESENT_END_NV};
    for (uint32_t i = 0; i < sizeof(markers) / sizeof(markers[0]); ++i) {
        uint32_t mapped = 0;
        if (!GetLatencyMarkerVK(markers[i], mapped) || mapped != native[i])
            return 1;
    }
    uint32_t marker = 0;
    if (GetLatencyMarkerVK(LatencyMarker::PC_LATENCY_PING, marker) || GetLatencyMarkerVK(LatencyMarker::MAX_NUM, marker))
        return 1;
    std::puts("Reflex native marker/queue ABI: PASS (NVAPI and Vulkan header/host checks)");
    return 0;
}
