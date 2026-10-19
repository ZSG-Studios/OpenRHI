#include "VendorFeaturesNative.h"

// FidelityFX uses the same loader and SDK lock as the upscaling adapter
#if OPENRHI_ENABLE_FFX_SDK && OPENRHI_ENABLE_D3D12_SUPPORT
static inline Result LoadFidelityFX(FidelityFXNative& impl) {
    impl.library = LoadSharedLibrary("amd_fidelityfx_loader_dx12.dll");

    if (!impl.library)
        return Result::UNSUPPORTED;

    impl.create = (PfnFfxCreateContext)GetSharedLibraryFunction(*impl.library, "ffxCreateContext");
    impl.destroy = (PfnFfxDestroyContext)GetSharedLibraryFunction(*impl.library, "ffxDestroyContext");
    impl.configure = (PfnFfxConfigure)GetSharedLibraryFunction(*impl.library, "ffxConfigure");
    impl.query = (PfnFfxQuery)GetSharedLibraryFunction(*impl.library, "ffxQuery");
    impl.dispatch = (PfnFfxDispatch)GetSharedLibraryFunction(*impl.library, "ffxDispatch");

    return impl.create && impl.destroy && impl.configure && impl.query && impl.dispatch ? Result::SUCCESS : Result::UNSUPPORTED;
}

FidelityFXNative::~FidelityFXNative() {
    ExclusiveScope lock(g_ffx.d3d12Lock);

    if (context && destroy)
        destroy(&context, nullptr);

    if (library)
        UnloadSharedLibrary(*library);
}

static Result OPENRHI_CALL CreateFidelityFXContext(Device& device, const FidelityFXDesc& desc, FidelityFXContext*& context) {
    context = nullptr;
    CoreInterface core = {};
    Result result = openrhiGetInterface(device, OPENRHI_INTERFACE(CoreInterface), &core);

    if (result != Result::SUCCESS)
        return result;

    if (core.GetDeviceDesc(device).graphicsAPI != GraphicsAPI::D3D12)
        return Result::UNSUPPORTED;

    auto* impl = Allocate<FidelityFXNative>(((DeviceBase&)device).GetAllocationCallbacks(), device, core);
    if (!impl)
        return Result::OUT_OF_MEMORY;

    result = LoadFidelityFX(*impl);

    impl->effect = desc.effect;

    if (result == Result::SUCCESS && desc.nativeDesc) {
        ExclusiveScope lock(g_ffx.d3d12Lock);
        impl->backend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
        impl->backend.device = (ID3D12Device*)core.GetDeviceNativeObject(&device);
        ffxCreateContextDescHeader* root = nullptr;

        if (desc.effect == FidelityFXEffect::RAY_REGENERATION) {
            impl->denoiser = *(const ffxCreateContextDescDenoiser*)desc.nativeDesc;
            root = &impl->denoiser.header;
        } else {
            impl->radiance = *(const ffxCreateContextDescRadianceCache*)desc.nativeDesc;
            root = &impl->radiance.header;
        }

        impl->backend.header.pNext = root->pNext;
        root->pNext = &impl->backend.header;
        result = FfxConvertError(impl->create(&impl->context, root, nullptr));

        if (result == Result::SUCCESS && !impl->context)
            result = Result::FAILURE;
    }

    if (result != Result::SUCCESS)
        Destroy(impl);
    else
        context = (FidelityFXContext*)impl;

    return result;
}

static void OPENRHI_CALL DestroyFidelityFXContext(FidelityFXContext* context) {
    Destroy((FidelityFXNative*)context);
}

static Result OPENRHI_CALL ConfigureFidelityFX(FidelityFXContext& context, const void* desc) {
    auto& impl = (FidelityFXNative&)context;
    ExclusiveScope lock(g_ffx.d3d12Lock);

    return FfxConvertError(impl.configure(impl.context ? &impl.context : nullptr, (const ffxConfigureDescHeader*)desc));
}

static Result OPENRHI_CALL QueryFidelityFX(FidelityFXContext& context, void* desc) {
    auto& impl = (FidelityFXNative&)context;
    ExclusiveScope lock(g_ffx.d3d12Lock);

    return FfxConvertError(impl.query(impl.context ? &impl.context : nullptr, (ffxQueryDescHeader*)desc));
}

template <typename T>
static inline Result DispatchFidelityFXNative(FidelityFXNative& impl, CommandBuffer& commandBuffer, const void* desc) {
    T native = *(const T*)desc;
    native.commandList = impl.core.GetCommandBufferNativeObject(&commandBuffer);

    return FfxConvertError(impl.dispatch(&impl.context, &native.header));
}

static Result OPENRHI_CALL CmdDispatchFidelityFX(CommandBuffer& commandBuffer, FidelityFXContext& context, const void* desc) {
    auto& impl = (FidelityFXNative&)context;
    if (!impl.context)
        return Result::INVALID_ARGUMENT;

    const auto type = ((const ffxDispatchDescHeader*)desc)->type;
    ExclusiveScope lock(g_ffx.d3d12Lock);

    if (impl.effect == FidelityFXEffect::RAY_REGENERATION) {
        if (type == FFX_API_DISPATCH_DESC_TYPE_DENOISER)
            return DispatchFidelityFXNative<ffxDispatchDescDenoiser>(impl, commandBuffer, desc);

    } else {
        if (type == FFX_API_DISPATCH_DESC_TYPE_RADIANCECACHE)
            return DispatchFidelityFXNative<ffxDispatchDescRadianceCache>(impl, commandBuffer, desc);

        if (type == FFX_API_DEBUG_DISPATCH_DESC_TYPE_RADIANCECACHE)
            return DispatchFidelityFXNative<ffxDebugDispatchDescRadianceCache>(impl, commandBuffer, desc);
    }

    return Result::INVALID_ARGUMENT;
}
#endif

Result openrhi::FillVendorFeaturesInterface(FidelityFXInterface& table) {
#if OPENRHI_ENABLE_FFX_SDK && OPENRHI_ENABLE_D3D12_SUPPORT
    table = {::CreateFidelityFXContext, ::DestroyFidelityFXContext, ::ConfigureFidelityFX, ::QueryFidelityFX, ::CmdDispatchFidelityFX};

    return Result::SUCCESS;
#else
    MaybeUnused(table);

    return Result::UNSUPPORTED;
#endif
}

#if OPENRHI_ENABLE_NRC_SDK
static NrcSharedState g_nrcD3D12;

static inline Result ConvertNrcStatus(nrc::Status status) {
    switch (status) {
        case nrc::Status::OK:
            return Result::SUCCESS;
        case nrc::Status::UnsupportedDriver:
        case nrc::Status::UnsupportedHardware:
            return Result::UNSUPPORTED;
        case nrc::Status::OutOfMemory:
        case nrc::Status::AllocationFailed:
            return Result::OUT_OF_MEMORY;
        case nrc::Status::WrongParameter:
        case nrc::Status::MemoryNotProvided:
        case nrc::Status::SDKVersionMismatch:
            return Result::INVALID_ARGUMENT;
        default:
            return Result::FAILURE;
    }
}

static inline bool EqualNrcSettings(const nrc::GlobalSettings& a, const nrc::GlobalSettings& b) {
    return a.majorVersion == b.majorVersion && a.minorVersion == b.minorVersion && a.loggerFn == b.loggerFn && a.memoryLoggerFn == b.memoryLoggerFn && a.allocatorFn == b.allocatorFn && a.deallocatorFn == b.deallocatorFn && a.enableGPUMemoryAllocation == b.enableGPUMemoryAllocation && a.enableDebugBuffers == b.enableDebugBuffers && a.maxNumFramesInFlight == b.maxNumFramesInFlight && a.depsDirectoryPath == b.depsDirectoryPath;
}

// Each macro branch preserves the SDK's native parameter and return types
#    if OPENRHI_ENABLE_D3D12_SUPPORT
#        define NRC_D3D12_CALL(impl, expression) \
            if ((impl).api == GraphicsAPI::D3D12) { \
                return ConvertNrcStatus((impl).d3d12->expression); \
            }
#    else
#        define NRC_D3D12_CALL(impl, expression)
#    endif

NrcNative::~NrcNative() {
    if (!acquired)
        return;

    ExclusiveScope lock(shared->lock);
#    if OPENRHI_ENABLE_D3D12_SUPPORT
    if (d3d12)
        nrc::d3d12::Context::Destroy(*d3d12);
#    endif

    if (--shared->references == 0) {
#    if OPENRHI_ENABLE_D3D12_SUPPORT
        if (api == GraphicsAPI::D3D12)
            nrc::d3d12::Shutdown();
#    endif
        shared->settings = {};

        if (shared->library) {
            UnloadSharedLibrary(*shared->library);
            shared->library = nullptr;
        }
    }
}

static Result OPENRHI_CALL ConfigureNeuralRadianceCache(NeuralRadianceCache& cache, const void* settings, const void* buffers, const char* filename) {
    auto& impl = (NrcNative&)cache;

    if (impl.inFrame)
        return Result::INVALID_ARGUMENT;

    NRC_D3D12_CALL(impl, Configure(*(const nrc::ContextSettings*)settings, (const nrc::d3d12::Buffers*)buffers, filename));

    return Result::UNSUPPORTED;
}

static inline Result AcquireNrcShared(NrcNative& impl, const nrc::GlobalSettings& settings) {
    auto& shared = *impl.shared;
    ExclusiveScope lock(shared.lock);

    if (shared.references && !EqualNrcSettings(shared.settings, settings))
        return Result::INVALID_ARGUMENT;

    if (!shared.references) {
        shared.library = LoadSharedLibrary("NRC_D3D12.dll");

        if (!shared.library)
            return Result::UNSUPPORTED;

        Result result = ConvertNrcStatus(nrc::d3d12::Initialize(settings));

        if (result != Result::SUCCESS) {
            UnloadSharedLibrary(*shared.library);
            shared.library = nullptr;

            return result;
        }
        shared.settings = settings;
    }
    ++shared.references;
    impl.acquired = true;

    return Result::SUCCESS;
}

static Result OPENRHI_CALL CreateNeuralRadianceCache(Device& device, const NeuralRadianceCacheDesc& desc, NeuralRadianceCache*& cache) {
    cache = nullptr;
    CoreInterface core = {};
    Result result = openrhiGetInterface(device, OPENRHI_INTERFACE(CoreInterface), &core);

    if (result != Result::SUCCESS)
        return result;

    const DeviceDesc& deviceDesc = core.GetDeviceDesc(device);

    if (deviceDesc.graphicsAPI != GraphicsAPI::D3D12 || deviceDesc.adapterDesc.vendor != Vendor::NVIDIA)
        return Result::UNSUPPORTED;

    auto* impl = Allocate<NrcNative>(((DeviceBase&)device).GetAllocationCallbacks(), device, core);
    if (!impl)
        return Result::OUT_OF_MEMORY;

    impl->api = deviceDesc.graphicsAPI;
    impl->shared = &g_nrcD3D12;
    result = AcquireNrcShared(*impl, *(const nrc::GlobalSettings*)desc.globalSettings);

    if (result == Result::SUCCESS) {
        ExclusiveScope lock(impl->shared->lock);
        result = ConvertNrcStatus(nrc::d3d12::Context::Create((ID3D12Device5*)core.GetDeviceNativeObject(&device), impl->d3d12));
    }

    if (result == Result::SUCCESS && !impl->d3d12)
        result = Result::FAILURE;

    if (result == Result::SUCCESS)
        result = ConfigureNeuralRadianceCache(*(NeuralRadianceCache*)impl, desc.contextSettings, desc.buffers, desc.networkConfigFilename);

    if (result != Result::SUCCESS)
        Destroy(impl);
    else
        cache = (NeuralRadianceCache*)impl;

    return result;
}

static void OPENRHI_CALL DestroyNeuralRadianceCache(NeuralRadianceCache* cache) {
    Destroy((NrcNative*)cache);
}

static Result OPENRHI_CALL GetNeuralRadianceCacheAllocationInfo(const Device& device, const NeuralRadianceCacheDesc& desc, void* info) {
    if (((const DeviceBase&)device).GetDesc().graphicsAPI != GraphicsAPI::D3D12)
        return Result::UNSUPPORTED;

    CoreInterface core = {};
    NrcNative impl((Device&)device, core);
    impl.api = GraphicsAPI::D3D12;
    impl.shared = &g_nrcD3D12;
    Result result = AcquireNrcShared(impl, *(const nrc::GlobalSettings*)desc.globalSettings);

    if (result == Result::SUCCESS) {
        ExclusiveScope lock(impl.shared->lock);
        result = ConvertNrcStatus(nrc::d3d12::Context::GetBuffersAllocationInfo(*(const nrc::ContextSettings*)desc.contextSettings, *(nrc::BuffersAllocationInfo*)info));
    }

    return result;
}

static Result OPENRHI_CALL GetNeuralRadianceCacheInitialBufferState(const NeuralRadianceCache& cache, const void* allocation, uint32_t& state) {
    const auto& impl = (const NrcNative&)cache;
    state = impl.d3d12->GetInitialBufferState(*(const nrc::AllocationInfo*)allocation);

    return Result::SUCCESS;
}

static const void* OPENRHI_CALL GetNeuralRadianceCacheBuffers(const NeuralRadianceCache& cache) {
    const auto& impl = (const NrcNative&)cache;
#    if OPENRHI_ENABLE_D3D12_SUPPORT
    if (impl.api == GraphicsAPI::D3D12)
        return impl.d3d12->GetBuffers();
#    endif

    return nullptr;
}

static Result OPENRHI_CALL GetNeuralRadianceCacheShaderConstants(const NeuralRadianceCache& cache, void* constants) {
    const auto& impl = (const NrcNative&)cache;
    NRC_D3D12_CALL(impl, PopulateShaderConstants(*(NrcConstants*)constants));

    return Result::UNSUPPORTED;
}

static Result OPENRHI_CALL CmdBeginNeuralRadianceCacheFrame(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, const void* settings) {
    auto& impl = (NrcNative&)cache;

    if (impl.inFrame)
        return Result::INVALID_ARGUMENT;

    Result result = Result::UNSUPPORTED;
#    if OPENRHI_ENABLE_D3D12_SUPPORT
    if (impl.api == GraphicsAPI::D3D12)
        result = ConvertNrcStatus(impl.d3d12->BeginFrame((ID3D12GraphicsCommandList4*)impl.core.GetCommandBufferNativeObject(&commandBuffer), *(const nrc::FrameSettings*)settings));
#    endif
    impl.inFrame = result == Result::SUCCESS;

    return result;
}

static Result OPENRHI_CALL CmdQueryAndTrainNeuralRadianceCache(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, float* loss) {
    auto& impl = (NrcNative&)cache;

    if (!impl.inFrame)
        return Result::INVALID_ARGUMENT;

    NRC_D3D12_CALL(impl, QueryAndTrain((ID3D12GraphicsCommandList4*)impl.core.GetCommandBufferNativeObject(&commandBuffer), loss));

    return Result::UNSUPPORTED;
}

static Result OPENRHI_CALL CmdResolveNeuralRadianceCache(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, uint64_t output) {
    auto& impl = (NrcNative&)cache;

    if (!impl.inFrame)
        return Result::INVALID_ARGUMENT;

    NRC_D3D12_CALL(impl, Resolve((ID3D12GraphicsCommandList4*)impl.core.GetCommandBufferNativeObject(&commandBuffer), (ID3D12Resource*)output));

    return Result::UNSUPPORTED;
}

static Result OPENRHI_CALL EndNeuralRadianceCacheFrame(Queue& queue, NeuralRadianceCache& cache) {
    auto& impl = (NrcNative&)cache;

    if (!impl.inFrame)
        return Result::INVALID_ARGUMENT;

    Result result = Result::UNSUPPORTED;
#    if OPENRHI_ENABLE_D3D12_SUPPORT
    if (impl.api == GraphicsAPI::D3D12)
        result = ConvertNrcStatus(impl.d3d12->EndFrame((ID3D12CommandQueue*)impl.core.GetQueueNativeObject(&queue)));
#    endif

    if (result == Result::SUCCESS)
        impl.inFrame = false;

    return result;
}

#    undef NRC_D3D12_CALL
#endif

Result openrhi::FillVendorFeaturesInterface(NeuralRadianceCacheInterface& table) {
#if OPENRHI_ENABLE_NRC_SDK
    table = {::CreateNeuralRadianceCache, ::DestroyNeuralRadianceCache, ::ConfigureNeuralRadianceCache, ::GetNeuralRadianceCacheAllocationInfo, ::GetNeuralRadianceCacheInitialBufferState, ::GetNeuralRadianceCacheBuffers, ::GetNeuralRadianceCacheShaderConstants, ::CmdBeginNeuralRadianceCacheFrame, ::CmdQueryAndTrainNeuralRadianceCache, ::CmdResolveNeuralRadianceCache, ::EndNeuralRadianceCacheFrame};

    return Result::SUCCESS;
#else
    MaybeUnused(table);

    return Result::UNSUPPORTED;
#endif
}

#if OPENRHI_ENABLE_ANTILAG2_SDK
static const GUID ANTI_LAG_DATA_GUID = {0x5083ae5b, 0x8070, 0x4fca, {0x8e, 0xe5, 0x35, 0x82, 0xdd, 0x36, 0x7d, 0x13}};

static inline Result UpdateAntiLagPrivateData(AntiLagNative& impl) {
    struct Data {
        AMD::AntiLag2DX12::Context* context;
        bool enabled;
    } data = {&impl.context, impl.enabled};

    return impl.swapChain ? GetResultFromHRESULT(impl.swapChain->SetPrivateData(ANTI_LAG_DATA_GUID, sizeof(data), &data)) : Result::SUCCESS;
}

AntiLagNative::~AntiLagNative() {
    if (swapChain) {
        swapChain->SetPrivateData(ANTI_LAG_DATA_GUID, 0, nullptr);
        swapChain->Release();
    }
    AMD::AntiLag2DX12::DeInitialize(&context);
}

static Result OPENRHI_CALL CreateAntiLagContext(Device& device, AntiLagContext*& context) {
    context = nullptr;
    CoreInterface core = {};
    Result result = openrhiGetInterface(device, OPENRHI_INTERFACE(CoreInterface), &core);

    if (result != Result::SUCCESS)
        return result;

    if (core.GetDeviceDesc(device).graphicsAPI != GraphicsAPI::D3D12 || core.GetDeviceDesc(device).adapterDesc.vendor != Vendor::AMD)
        return Result::UNSUPPORTED;

    auto* impl = Allocate<AntiLagNative>(((DeviceBase&)device).GetAllocationCallbacks(), device);
    if (!impl)
        return Result::OUT_OF_MEMORY;

    HRESULT hr = AMD::AntiLag2DX12::Initialize(&impl->context, (ID3D12Device*)core.GetDeviceNativeObject(&device));

    if (hr != S_OK) {
        Destroy(impl);

        return Result::UNSUPPORTED;
    }
    context = (AntiLagContext*)impl;

    return Result::SUCCESS;
}

static void OPENRHI_CALL DestroyAntiLagContext(AntiLagContext* context) {
    Destroy((AntiLagNative*)context);
}

static Result OPENRHI_CALL UpdateAntiLag(AntiLagContext& context, bool enabled, uint32_t maxFPS) {
    auto& impl = (AntiLagNative&)context;
    HRESULT hr = AMD::AntiLag2DX12::Update(&impl.context, enabled, maxFPS);

    if (hr != S_OK)
        return GetResultFromHRESULT(hr);

    const bool changed = impl.enabled != enabled;
    impl.enabled = enabled;

    return changed ? UpdateAntiLagPrivateData(impl) : Result::SUCCESS;
}

static Result OPENRHI_CALL MarkAntiLagEndOfRendering(AntiLagContext& context) {
    return GetResultFromHRESULT(AMD::AntiLag2DX12::MarkEndOfFrameRendering(&((AntiLagNative&)context).context));
}

static Result OPENRHI_CALL SetAntiLagFrameType(AntiLagContext& context, bool interpolated) {
    return GetResultFromHRESULT(AMD::AntiLag2DX12::SetFrameGenFrameType(&((AntiLagNative&)context).context, interpolated));
}

static Result OPENRHI_CALL AttachAntiLagSwapChain(AntiLagContext& context, void* nativeSwapChain) {
    auto& impl = (AntiLagNative&)context;
    auto* next = (IDXGISwapChain*)nativeSwapChain;

    if (next)
        next->AddRef();

    if (impl.swapChain) {
        impl.swapChain->SetPrivateData(ANTI_LAG_DATA_GUID, 0, nullptr);
        impl.swapChain->Release();
    }
    impl.swapChain = next;

    return UpdateAntiLagPrivateData(impl);
}
#endif

Result openrhi::FillVendorFeaturesInterface(AntiLagInterface& table) {
#if OPENRHI_ENABLE_ANTILAG2_SDK
    table = {::CreateAntiLagContext, ::DestroyAntiLagContext, ::UpdateAntiLag, ::MarkAntiLagEndOfRendering, ::SetAntiLagFrameType, ::AttachAntiLagSwapChain};

    return Result::SUCCESS;
#else
    MaybeUnused(table);

    return Result::UNSUPPORTED;
#endif
}
