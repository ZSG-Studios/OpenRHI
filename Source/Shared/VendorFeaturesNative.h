#pragma once

#if OPENRHI_ENABLE_FFX_SDK && OPENRHI_ENABLE_D3D12_SUPPORT
#    include "ffx_denoiser.h"
#    include "ffx_radiancecache.h"

static_assert(FFX_DENOISER_VERSION == FFX_DENOISER_MAKE_VERSION(1, 2, 0), "OpenRHI requires Ray Regeneration 1.2.0");
static_assert(FFX_RADIANCECACHE_VERSION == FFX_RADIANCECACHE_MAKE_VERSION(0, 9, 0), "OpenRHI requires radiance cache 0.9.0");

struct FidelityFXNative;
static inline Result LoadFidelityFX(FidelityFXNative& impl);
static Result OPENRHI_CALL CreateFidelityFXContext(Device& device, const FidelityFXDesc& desc, FidelityFXContext*& context);
static Result OPENRHI_CALL ConfigureFidelityFX(FidelityFXContext& context, const void* desc);
static Result OPENRHI_CALL QueryFidelityFX(FidelityFXContext& context, void* desc);
template <typename T>
static inline Result DispatchFidelityFXNative(FidelityFXNative& impl, CommandBuffer& commandBuffer, const void* desc);
static Result OPENRHI_CALL CmdDispatchFidelityFX(CommandBuffer& commandBuffer, FidelityFXContext& context, const void* desc);

struct FidelityFXNative : public openrhi::DebugNameBase {
    inline FidelityFXNative(openrhi::Device& d, const openrhi::CoreInterface& c) : device(d), core(c) {
    }

    ~FidelityFXNative();

    openrhi::Device& GetDevice() {
        return device;
    }

private:
    friend Result LoadFidelityFX(FidelityFXNative& impl);
    friend Result OPENRHI_CALL CreateFidelityFXContext(Device& device, const FidelityFXDesc& desc, FidelityFXContext*& context);
    friend Result OPENRHI_CALL ConfigureFidelityFX(FidelityFXContext& context, const void* desc);
    friend Result OPENRHI_CALL QueryFidelityFX(FidelityFXContext& context, void* desc);
    template <typename T>
    friend Result DispatchFidelityFXNative(FidelityFXNative& impl, CommandBuffer& commandBuffer, const void* desc);
    friend Result OPENRHI_CALL CmdDispatchFidelityFX(CommandBuffer& commandBuffer, FidelityFXContext& context, const void* desc);

    openrhi::Device& device;
    openrhi::CoreInterface core;
    openrhi::Library* library = nullptr;
    ffxContext context = nullptr;
    PfnFfxCreateContext create = nullptr;
    PfnFfxDestroyContext destroy = nullptr;
    PfnFfxConfigure configure = nullptr;
    PfnFfxQuery query = nullptr;
    PfnFfxDispatch dispatch = nullptr;
    ffxCreateBackendDX12Desc backend = {};
    ffxCreateContextDescDenoiser denoiser = {};
    ffxCreateContextDescRadianceCache radiance = {};
    openrhi::FidelityFXEffect effect = {};
};
#endif

#if OPENRHI_ENABLE_NRC_SDK
#    if OPENRHI_ENABLE_D3D12_SUPPORT
#        include "NrcD3d12.h"
static_assert(NRC_VERSION_MAJOR == 0 && NRC_VERSION_MINOR == 16, "OpenRHI requires NRC 0.16");
#    endif

struct NrcSharedState {
    Lock lock;
    uint32_t references = 0;
    openrhi::Library* library = nullptr;
    nrc::GlobalSettings settings = {};
};

struct NrcNative;
static Result OPENRHI_CALL ConfigureNeuralRadianceCache(NeuralRadianceCache& cache, const void* settings, const void* buffers, const char* filename);
static inline Result AcquireNrcShared(NrcNative& impl, const nrc::GlobalSettings& settings);
static Result OPENRHI_CALL CreateNeuralRadianceCache(Device& device, const NeuralRadianceCacheDesc& desc, NeuralRadianceCache*& cache);
static Result OPENRHI_CALL GetNeuralRadianceCacheAllocationInfo(const Device& device, const NeuralRadianceCacheDesc& desc, void* info);
static Result OPENRHI_CALL GetNeuralRadianceCacheInitialBufferState(const NeuralRadianceCache& cache, const void* allocation, uint32_t& state);
static const void* OPENRHI_CALL GetNeuralRadianceCacheBuffers(const NeuralRadianceCache& cache);
static Result OPENRHI_CALL GetNeuralRadianceCacheShaderConstants(const NeuralRadianceCache& cache, void* constants);
static Result OPENRHI_CALL CmdBeginNeuralRadianceCacheFrame(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, const void* settings);
static Result OPENRHI_CALL CmdQueryAndTrainNeuralRadianceCache(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, float* loss);
static Result OPENRHI_CALL CmdResolveNeuralRadianceCache(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, uint64_t output);
static Result OPENRHI_CALL EndNeuralRadianceCacheFrame(Queue& queue, NeuralRadianceCache& cache);

struct NrcNative : public openrhi::DebugNameBase {
    inline NrcNative(openrhi::Device& d, const openrhi::CoreInterface& c) : device(d), core(c) {
    }

    ~NrcNative();

    openrhi::Device& GetDevice() {
        return device;
    }

private:
    friend Result OPENRHI_CALL ConfigureNeuralRadianceCache(NeuralRadianceCache& cache, const void* settings, const void* buffers, const char* filename);
    friend Result AcquireNrcShared(NrcNative& impl, const nrc::GlobalSettings& settings);
    friend Result OPENRHI_CALL CreateNeuralRadianceCache(Device& device, const NeuralRadianceCacheDesc& desc, NeuralRadianceCache*& cache);
    friend Result OPENRHI_CALL GetNeuralRadianceCacheAllocationInfo(const Device& device, const NeuralRadianceCacheDesc& desc, void* info);
    friend Result OPENRHI_CALL GetNeuralRadianceCacheInitialBufferState(const NeuralRadianceCache& cache, const void* allocation, uint32_t& state);
    friend const void* OPENRHI_CALL GetNeuralRadianceCacheBuffers(const NeuralRadianceCache& cache);
    friend Result OPENRHI_CALL GetNeuralRadianceCacheShaderConstants(const NeuralRadianceCache& cache, void* constants);
    friend Result OPENRHI_CALL CmdBeginNeuralRadianceCacheFrame(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, const void* settings);
    friend Result OPENRHI_CALL CmdQueryAndTrainNeuralRadianceCache(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, float* loss);
    friend Result OPENRHI_CALL CmdResolveNeuralRadianceCache(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, uint64_t output);
    friend Result OPENRHI_CALL EndNeuralRadianceCacheFrame(Queue& queue, NeuralRadianceCache& cache);

    openrhi::Device& device;
    openrhi::CoreInterface core;
    openrhi::GraphicsAPI api = {};
    NrcSharedState* shared = nullptr;
    bool acquired = false;
    bool inFrame = false;
#    if OPENRHI_ENABLE_D3D12_SUPPORT
    nrc::d3d12::Context* d3d12 = nullptr;
#    endif
};
#endif

#if OPENRHI_ENABLE_ANTILAG2_SDK
#    include "ffx_antilag2_dx12.h"

struct AntiLagNative;
static inline Result UpdateAntiLagPrivateData(AntiLagNative& impl);
static Result OPENRHI_CALL CreateAntiLagContext(Device& device, AntiLagContext*& context);
static Result OPENRHI_CALL UpdateAntiLag(AntiLagContext& context, bool enabled, uint32_t maxFPS);
static Result OPENRHI_CALL MarkAntiLagEndOfRendering(AntiLagContext& context);
static Result OPENRHI_CALL SetAntiLagFrameType(AntiLagContext& context, bool interpolated);
static Result OPENRHI_CALL AttachAntiLagSwapChain(AntiLagContext& context, void* nativeSwapChain);

struct AntiLagNative : public openrhi::DebugNameBase {
    inline AntiLagNative(openrhi::Device& d) : device(d) {
    }

    ~AntiLagNative();

    openrhi::Device& GetDevice() {
        return device;
    }

private:
    friend Result UpdateAntiLagPrivateData(AntiLagNative& impl);
    friend Result OPENRHI_CALL CreateAntiLagContext(Device& device, AntiLagContext*& context);
    friend Result OPENRHI_CALL UpdateAntiLag(AntiLagContext& context, bool enabled, uint32_t maxFPS);
    friend Result OPENRHI_CALL MarkAntiLagEndOfRendering(AntiLagContext& context);
    friend Result OPENRHI_CALL SetAntiLagFrameType(AntiLagContext& context, bool interpolated);
    friend Result OPENRHI_CALL AttachAntiLagSwapChain(AntiLagContext& context, void* nativeSwapChain);

    openrhi::Device& device;
    AMD::AntiLag2DX12::Context context = {};
    IDXGISwapChain* swapChain = nullptr;
    bool enabled = false;
};
#endif
