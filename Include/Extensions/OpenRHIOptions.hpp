// OpenRHI-owned, UI-independent rendering-option metadata and read-only startup queries
#pragma once

#include "OpenRHIUpscaler.h"
#include "OpenRHILowLatency.h"
#include "OpenRHIVendorFeatures.h"
#include "OpenRHIFrameGeneration.h"
#include <array>

namespace openrhi {

enum class RenderOption : uint8_t {
    NIS, FSR, XESS, DLSS, DLSS_RR, METALFX_SPATIAL, METALFX_TEMPORAL, METALFX_DENOISED, ANF,
    ANTI_LAG_2, REFLEX, AMD_RAY_REGENERATION, AMD_RADIANCE_CACHE, NVIDIA_NRC, FRAME_GENERATION,
    MAX_NUM
};

enum RenderOptionInput : uint32_t {
    OPTION_COLOR = 1u << 0,
    OPTION_DEPTH = 1u << 1,
    OPTION_MOTION = 1u << 2,
    OPTION_JITTER_CAMERA = 1u << 3,
    OPTION_DENOISER_SIGNALS = 1u << 4,
    OPTION_PATH_TRACER_PRODUCERS = 1u << 5,
    OPTION_INPUT_HOOK = 1u << 6,
    OPTION_SUBMISSION_HOOK = 1u << 7,
    OPTION_PRESENT_HOOK = 1u << 8
};

struct RenderOptionDesc {
    const char* name;
    const char* interfaceName;
    const char* buildOption;
    uint32_t requiredInputs;
    bool publicExecutionPath;
};

inline const std::array<RenderOptionDesc, size_t(RenderOption::MAX_NUM)>& GetOpenRHIRenderOptionDescs() {
    static constexpr uint32_t temporal = OPTION_COLOR | OPTION_DEPTH | OPTION_MOTION | OPTION_JITTER_CAMERA;
    static constexpr std::array<RenderOptionDesc, size_t(RenderOption::MAX_NUM)> descs = {{
        {"NVIDIA Image Scaling", "UpscalerInterface", "OPENRHI_ENABLE_NIS_SDK", OPTION_COLOR, true},
        {"AMD FSR Super Resolution", "UpscalerInterface", "OPENRHI_ENABLE_FFX_SDK", temporal, true},
        {"Intel XeSS", "UpscalerInterface", "OPENRHI_ENABLE_XESS_SDK", temporal, true},
        {"NVIDIA DLSS Super Resolution", "UpscalerInterface", "OPENRHI_ENABLE_NGX_SDK", temporal, true},
        {"NVIDIA DLSS Ray Reconstruction", "UpscalerInterface", "OPENRHI_ENABLE_NGX_SDK", temporal | OPTION_DENOISER_SIGNALS, true},
        {"MetalFX Spatial", "UpscalerInterface", "OPENRHI_ENABLE_METAL_SUPPORT", OPTION_COLOR, true},
        {"MetalFX Temporal", "UpscalerInterface", "OPENRHI_ENABLE_METAL_SUPPORT", temporal, true},
        {"MetalFX Denoised", "UpscalerInterface", "OPENRHI_ENABLE_METAL_SUPPORT", temporal | OPTION_DENOISER_SIGNALS, true},
        {"Adreno Neural Fusion", "UpscalerInterface", "OPENRHI_ENABLE_ANF_SDK", temporal, true},
        {"AMD Anti-Lag 2", "AntiLagInterface", "OPENRHI_ENABLE_ANTILAG2_SDK", OPTION_INPUT_HOOK | OPTION_SUBMISSION_HOOK | OPTION_PRESENT_HOOK, true},
        {"NVIDIA Reflex", "LowLatencyInterface", "OPENRHI_ENABLE_NVAPI / Vulkan low-latency extensions", OPTION_INPUT_HOOK | OPTION_SUBMISSION_HOOK | OPTION_PRESENT_HOOK, true},
        {"AMD Ray Regeneration", "FidelityFXInterface", "OPENRHI_ENABLE_FFX_SDK", temporal | OPTION_DENOISER_SIGNALS, true},
        {"AMD radiance cache", "FidelityFXInterface", "OPENRHI_ENABLE_FFX_SDK", OPTION_PATH_TRACER_PRODUCERS, true},
        {"NVIDIA neural radiance cache", "NeuralRadianceCacheInterface", "OPENRHI_ENABLE_NRC_SDK", OPTION_PATH_TRACER_PRODUCERS | OPTION_SUBMISSION_HOOK, true},
        {"AMD frame generation", "FrameGenerationInterface", "OPENRHI_ENABLE_FFX_FRAME_GENERATION", temporal | OPTION_PRESENT_HOOK, true},
    }};

    return descs;
}

// A successful table query describes interface availability; creation/dispatch/active state remain separate
struct RenderOptionSupport {
    Result interfaceResult = Result::UNSUPPORTED;
    bool upscalerReportedSupported = false;
};

struct RenderOptionsSnapshot {
    bool nativeDevice = false;
    std::array<RenderOptionSupport, size_t(RenderOption::MAX_NUM)> options = {};
};

// Explicit startup/settings-refresh query; does not create contexts, dispatch, or change application settings
inline Result QueryOpenRHIRenderOptions(const Device& device, RenderOptionsSnapshot& snapshot) {
    snapshot = {};
    CoreInterface core = {};
    Result result = openrhiGetInterface(device, OPENRHI_INTERFACE(CoreInterface), &core);

    if (result != Result::SUCCESS)
        return result;

    snapshot.nativeDevice = core.GetDeviceDesc(device).graphicsAPI != GraphicsAPI::NONE;
    UpscalerInterface upscaler = {};
    const Result upscalerResult = openrhiGetInterface(device, OPENRHI_INTERFACE(UpscalerInterface), &upscaler);
    constexpr UpscalerType types[] = {UpscalerType::NIS, UpscalerType::FSR, UpscalerType::XESS, UpscalerType::DLSR, UpscalerType::DLRR, UpscalerType::METALFX_SPATIAL, UpscalerType::METALFX_TEMPORAL, UpscalerType::METALFX_DENOISED, UpscalerType::ANF};

    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        snapshot.options[i].interfaceResult = upscalerResult;
        snapshot.options[i].upscalerReportedSupported = snapshot.nativeDevice && upscalerResult == Result::SUCCESS && upscaler.IsUpscalerSupported(device, types[i]);
    }

    AntiLagInterface antiLag = {};
    LowLatencyInterface latency = {};
    FidelityFXInterface fidelityFX = {};
    NeuralRadianceCacheInterface nrc = {};
    snapshot.options[size_t(RenderOption::ANTI_LAG_2)].interfaceResult = openrhiGetInterface(device, OPENRHI_INTERFACE(AntiLagInterface), &antiLag);
    snapshot.options[size_t(RenderOption::REFLEX)].interfaceResult = openrhiGetInterface(device, OPENRHI_INTERFACE(LowLatencyInterface), &latency);
    const Result fidelityResult = openrhiGetInterface(device, OPENRHI_INTERFACE(FidelityFXInterface), &fidelityFX);
    snapshot.options[size_t(RenderOption::AMD_RAY_REGENERATION)].interfaceResult = fidelityResult;
    snapshot.options[size_t(RenderOption::AMD_RADIANCE_CACHE)].interfaceResult = fidelityResult;
    snapshot.options[size_t(RenderOption::NVIDIA_NRC)].interfaceResult = openrhiGetInterface(device, OPENRHI_INTERFACE(NeuralRadianceCacheInterface), &nrc);
    FrameGenerationInterface generation = {};
    snapshot.options[size_t(RenderOption::FRAME_GENERATION)].interfaceResult = openrhiGetInterface(device, OPENRHI_INTERFACE(FrameGenerationInterface), &generation);

    return Result::SUCCESS;
}

} // namespace openrhi
