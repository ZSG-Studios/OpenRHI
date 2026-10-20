// Shared public facade over the pinned D3D12 frame-generation provider
#if OPENRHI_ENABLE_FFX_FRAME_GENERATION
static_assert(FFX_FRAMEGENERATION_VERSION == FFX_FRAMEGENERATION_MAKE_VERSION(4, 0, 1), "OpenRHI FG API pin");
static_assert(FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION == FFX_FRAMEGENERATION_SWAPCHAIN_DX12_MAKE_VERSION(3, 1, 7), "OpenRHI FG swapchain API pin");

namespace openrhi {

static Result OPENRHI_CALL EnumerateFrameGenerationProviders(const Device& device, FrameGenerationProvider* providers, uint32_t& providerNum) {
    CoreInterface core = {};
    Result result = openrhiGetInterface(device, OPENRHI_INTERFACE(CoreInterface), &core);

    if (result != Result::SUCCESS || core.GetDeviceDesc(device).graphicsAPI != GraphicsAPI::D3D12) {
        providerNum = 0;

        return Result::UNSUPPORTED;
    }

    Library* library = LoadSharedLibrary("amd_fidelityfx_loader_dx12.dll");

    if (!library) {
        providerNum = 0;

        return Result::UNSUPPORTED;
    }

    auto query = (PfnFfxQuery)GetSharedLibraryFunction(*library, "ffxQuery");
    uint64_t count = providers ? providerNum : 0;
    DeviceBase& base = (DeviceBase&)device;
    Scratch<uint64_t> ids = OPENRHI_ALLOCATE_SCRATCH(base, uint64_t, count);
    Scratch<const char*> names = OPENRHI_ALLOCATE_SCRATCH(base, const char*, count);
    ffxQueryDescGetVersions native = {};
    native.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    native.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;
    native.device = (void*)core.GetDeviceNativeObject(&device);
    native.outputCount = &count;
    native.versionIds = (providers && count) ? (uint64_t*)ids : nullptr;
    native.versionNames = (providers && count) ? (const char**)names : nullptr;
    {
        ExclusiveScope lock(g_ffx.d3d12Lock);
        result = query ? FfxConvertError(query(nullptr, &native.header)) : Result::UNSUPPORTED;

        if (result == Result::SUCCESS && providers) {
            const uint32_t copied = uint32_t(std::min(count, uint64_t(providerNum)));

            for (uint32_t i = 0; i < copied; i++) {
                providers[i].id = ids[i];
                snprintf(providers[i].name, sizeof(providers[i].name), "%s", names[i] ? names[i] : "");
            }
        }
    }
    providerNum = uint32_t(std::min(count, uint64_t(UINT32_MAX)));
    UnloadSharedLibrary(*library);

    return result;
}

FrameGenerationImpl::~FrameGenerationImpl() {
    // SDK shutdown deconfigures callbacks/UI before releasing interpolation resources
    if (m_Context && m_SwapContext && m_SwapChain) {
        FrameGenerationConfigureDesc disabled = {};
        disabled.frameId = m_Stats.lastFrameId;
        Configure(disabled);
    }

    for (Texture* texture : m_Textures)
        m_Core.DestroyTexture(texture);

    if (m_Destroy) {
        ExclusiveScope lock(g_ffx.d3d12Lock);

        if (m_Context)
            m_Destroy(&m_Context, nullptr);

        if (m_SwapContext)
            m_Destroy(&m_SwapContext, nullptr);
    }

    // Creation returns a caller reference in addition to the SDK context's reference
    if (m_SwapChain)
        m_SwapChain->Release();

    // Each acquired waitable handle is separately caller-owned
    if (m_Waitable)
        CloseHandle(m_Waitable);

    if (m_Factory)
        m_Factory->Release();

    if (m_Library)
        UnloadSharedLibrary(*m_Library);
}

Result FrameGenerationImpl::Create(const FrameGenerationDesc& desc) {
    m_Desc = desc;
    m_Library = LoadSharedLibrary("amd_fidelityfx_loader_dx12.dll");

    if (!m_Library)
        return Result::UNSUPPORTED;

    m_Create = (PfnFfxCreateContext)GetSharedLibraryFunction(*m_Library, "ffxCreateContext");
    m_Destroy = (PfnFfxDestroyContext)GetSharedLibraryFunction(*m_Library, "ffxDestroyContext");
    m_Configure = (PfnFfxConfigure)GetSharedLibraryFunction(*m_Library, "ffxConfigure");
    m_Query = (PfnFfxQuery)GetSharedLibraryFunction(*m_Library, "ffxQuery");
    m_Dispatch = (PfnFfxDispatch)GetSharedLibraryFunction(*m_Library, "ffxDispatch");

    if (!m_Create || !m_Destroy || !m_Configure || !m_Query || !m_Dispatch)
        return Result::UNSUPPORTED;

    if (desc.enableDebugChecking) {
        ffxConfigureDescGlobalDebug debug = {};
        debug.header.type = FFX_API_CONFIGURE_DESC_TYPE_GLOBALDEBUG;
        debug.effectId = FFX_API_EFFECT_ID_FRAMEGENERATION;
        debug.fpMessage = [](uint32_t type, const wchar_t* message) { OPENRHI_FFX_DEBUG_LOG(type, message); };
        debug.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_VERBOSE;
        ExclusiveScope lock(g_ffx.d3d12Lock);
        m_Configure(nullptr, &debug.header);
    }

    m_Backend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
    m_Backend.device = (ID3D12Device*)m_Core.GetDeviceNativeObject(&m_Device);
    m_Version.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_VERSION;
    m_Version.version = FFX_FRAMEGENERATION_VERSION;
    m_Backend.header.pNext = &m_Version.header;
    m_ContextDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;
    m_ContextDesc.header.pNext = &m_Backend.header;

    if (desc.providerId) {
        m_Override.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
        m_Override.versionId = desc.providerId;
        m_Version.header.pNext = &m_Override.header;
    }

    constexpr Format formats[] = {Format::RGBA16_SFLOAT, Format::RGBA8_UNORM, Format::R10_G10_B10_A2_UNORM, Format::R10_G10_B10_A2_UNORM};
    const Format format = formats[uint32_t(desc.swapChain.format)];
    m_ContextDesc.backBufferFormat = FfxConvertFormat(format);
    m_ContextDesc.displaySize = {desc.swapChain.width, desc.swapChain.height};
    m_ContextDesc.maxRenderSize = {desc.maxRenderResolution.w, desc.maxRenderResolution.h};

    if (desc.flags & UpscalerBits::HDR)
        m_ContextDesc.flags |= FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE;
    if (desc.flags & UpscalerBits::DEPTH_INVERTED)
        m_ContextDesc.flags |= FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED;
    if (desc.flags & UpscalerBits::DEPTH_INFINITE)
        m_ContextDesc.flags |= FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE;
    if (desc.flags & UpscalerBits::MV_UPSCALED)
        m_ContextDesc.flags |= FFX_FRAMEGENERATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS;
    if (desc.flags & UpscalerBits::MV_JITTERED)
        m_ContextDesc.flags |= FFX_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;
    if (desc.enableDebugChecking)
        m_ContextDesc.flags |= FFX_FRAMEGENERATION_ENABLE_DEBUG_CHECKING;
    {
        ExclusiveScope lock(g_ffx.d3d12Lock);
        Result result = FfxConvertError(m_Create(&m_Context, &m_ContextDesc.header, nullptr));

        if (result != Result::SUCCESS)
            return result;

        if (!m_Context)
            return Result::FAILURE;

        ffxQueryGetProviderVersion provider = {};
        provider.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;

        if (m_Query(&m_Context, &provider.header) == FFX_API_RETURN_OK) {
            m_Stats.provider.id = provider.versionId;
            snprintf(m_Stats.provider.name, sizeof(m_Stats.provider.name), "%s", provider.versionName ? provider.versionName : "");
        }
    }

    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_Factory));

    if (FAILED(hr))
        return GetResultFromHRESULT(hr);

    if (desc.swapChain.flags & SwapChainBits::ALLOW_TEARING) {
        IDXGIFactory5* factory5 = nullptr;

        if (SUCCEEDED(m_Factory->QueryInterface(IID_PPV_ARGS(&factory5)))) {
            BOOL supported = FALSE;
            m_AllowTearing = SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supported, sizeof(supported))) && supported;
            factory5->Release();
        }
    }

    m_DxgiDesc.Width = desc.swapChain.width;
    m_DxgiDesc.Height = desc.swapChain.height;
    m_DxgiDesc.Format = (DXGI_FORMAT)openrhiConvertOpenRHIFormatToDXGI(format);
    m_DxgiDesc.SampleDesc.Count = 1;
    m_DxgiDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    m_DxgiDesc.BufferCount = desc.swapChain.textureNum;
    m_DxgiDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    m_DxgiDesc.Scaling = DXGI_SCALING_STRETCH;
    m_DxgiDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    if (m_AllowTearing)
        m_DxgiDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    if (desc.swapChain.flags & SwapChainBits::WAITABLE)
        m_DxgiDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    m_SwapVersion.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12;
    m_SwapVersion.version = FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION;
    m_SwapDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12;
    m_SwapDesc.header.pNext = &m_SwapVersion.header;
    m_SwapDesc.swapchain = &m_SwapChain;
    m_SwapDesc.hwnd = (HWND)desc.swapChain.window.windows.hwnd;
    m_SwapDesc.desc = &m_DxgiDesc;
    m_SwapDesc.dxgiFactory = m_Factory;
    m_SwapDesc.gameQueue = (ID3D12CommandQueue*)m_Core.GetQueueNativeObject(desc.swapChain.queue);
    {
        ExclusiveScope lock(g_ffx.d3d12Lock);
        Result result = FfxConvertError(m_Create(&m_SwapContext, &m_SwapDesc.header, nullptr));

        if (result != Result::SUCCESS)
            return result;
    }

    if (!m_SwapContext || !m_SwapChain)
        return Result::FAILURE;

    hr = m_Factory->MakeWindowAssociation(m_SwapDesc.hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (FAILED(hr))
        return GetResultFromHRESULT(hr);

    constexpr DXGI_COLOR_SPACE_TYPE colorSpaces[] = {DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709, DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020};
    const DXGI_COLOR_SPACE_TYPE colorSpace = colorSpaces[uint32_t(desc.swapChain.format)];
    UINT colorSpaceSupport = 0;
    hr = m_SwapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport);

    if (FAILED(hr) || !(colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
        return Result::UNSUPPORTED;

    hr = m_SwapChain->SetColorSpace1(colorSpace);

    if (FAILED(hr))
        return GetResultFromHRESULT(hr);

    if (desc.swapChain.flags & SwapChainBits::WAITABLE) {
        hr = m_SwapChain->SetMaximumFrameLatency(desc.swapChain.queuedFrameNum ? desc.swapChain.queuedFrameNum : 1);

        if (FAILED(hr))
            return GetResultFromHRESULT(hr);
        m_Waitable = m_SwapChain->GetFrameLatencyWaitableObject();

        if (!m_Waitable)
            return Result::FAILURE;
    }

    DXGI_SWAP_CHAIN_DESC1 actual = {};
    hr = m_SwapChain->GetDesc1(&actual);

    if (FAILED(hr))
        return GetResultFromHRESULT(hr);

    WrapperD3D12Interface wrapper = {};
    Result result = openrhiGetInterface(m_Device, OPENRHI_INTERFACE(WrapperD3D12Interface), &wrapper);

    if (result != Result::SUCCESS)
        return result;

    for (uint32_t i = 0; i < actual.BufferCount; i++) {
        ID3D12Resource* nativeTexture = nullptr;
        hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&nativeTexture));

        if (FAILED(hr))
            return GetResultFromHRESULT(hr);

        TextureD3D12Desc native = {};
        native.d3d12Resource = nativeTexture;
        Texture* texture = nullptr;
        result = wrapper.CreateTextureD3D12(m_Device, native, texture);
        nativeTexture->Release();

        if (result != Result::SUCCESS)
            return result;

        m_Textures.push_back(texture);
    }

    return Result::SUCCESS;
}

Result FrameGenerationImpl::Configure(const FrameGenerationConfigureDesc& desc) {
    if (desc.enabled && (!desc.frameId || desc.frameId <= m_Stats.lastFrameId || m_PreparedFrame))
        return Result::INVALID_ARGUMENT;

    m_UiConfig = {};
    m_UiConfig.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_REGISTERUIRESOURCE_DX12;
    m_UiConfig.uiResource = FfxGetResource(m_Core, {desc.ui, nullptr}, false, true);
    m_UiConfig.flags = FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING;

    if (desc.uiPremultiplied)
        m_UiConfig.flags |= FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_USE_PREMUL_ALPHA;

    ExclusiveScope lock(g_ffx.d3d12Lock);
    Result result = FfxConvertError(m_Configure(&m_SwapContext, &m_UiConfig.header));

    if (result != Result::SUCCESS)
        return result;

    m_Config = {};
    m_Config.header.type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;
    m_Config.swapChain = m_SwapChain;
    m_Config.frameGenerationCallback = Dispatch;
    m_Config.frameGenerationCallbackUserContext = this;
    m_Config.frameGenerationEnabled = desc.enabled;
    m_Config.HUDLessColor = FfxGetResource(m_Core, {desc.hudlessColor, nullptr}, false, true);
    m_Config.generationRect = {0, 0, m_Desc.swapChain.width, m_Desc.swapChain.height};
    m_Config.frameID = desc.frameId;
    result = FfxConvertError(m_Configure(&m_Context, &m_Config.header));

    if (result == Result::SUCCESS) {
        m_Stats.configuredEnabled = desc.enabled;

        if (desc.frameId)
            m_Stats.lastFrameId = desc.frameId;

        if (!desc.enabled)
            m_PreparedFrame = 0;
    }

    return result;
}

Result FrameGenerationImpl::Prepare(CommandBuffer& commandBuffer, const FrameGenerationPrepareDesc& desc) {
    if (!m_Stats.configuredEnabled || desc.frameId != m_Stats.lastFrameId || m_PreparedFrame)
        return Result::INVALID_ARGUMENT;

    ffxDispatchDescFrameGenerationPrepareV2 native = {};
    native.header.type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_V2;
    native.frameID = desc.frameId;
    native.commandList = m_Core.GetCommandBufferNativeObject(&commandBuffer);
    native.renderSize = {desc.renderResolution.w, desc.renderResolution.h};
    native.jitterOffset = {desc.cameraJitter.x, desc.cameraJitter.y};
    native.motionVectorScale = {desc.motionVectorScale.x, desc.motionVectorScale.y};
    native.frameTimeDelta = desc.frameTimeMs;
    native.reset = desc.reset;
    native.cameraNear = desc.cameraNear;
    native.cameraFar = (m_Desc.flags & UpscalerBits::DEPTH_INFINITE) ? FLT_MAX : desc.cameraFar;

    if (m_Desc.flags & UpscalerBits::DEPTH_INVERTED)
        std::swap(native.cameraNear, native.cameraFar);
    native.cameraFovAngleVertical = desc.verticalFov;
    native.viewSpaceToMetersFactor = desc.viewSpaceToMetersFactor;
    native.depth = FfxGetResource(m_Core, {desc.depth, nullptr}, false, true);
    native.motionVectors = FfxGetResource(m_Core, {desc.motionVectors, nullptr}, false, true);
    memcpy(native.cameraPosition, desc.cameraPosition, sizeof(native.cameraPosition));
    memcpy(native.cameraUp, desc.cameraUp, sizeof(native.cameraUp));
    memcpy(native.cameraRight, desc.cameraRight, sizeof(native.cameraRight));
    memcpy(native.cameraForward, desc.cameraForward, sizeof(native.cameraForward));
    ExclusiveScope lock(g_ffx.d3d12Lock);
    Result result = FfxConvertError(m_Dispatch(&m_Context, &native.header));

    if (result == Result::SUCCESS) {
        m_PreparedFrame = desc.frameId;
        m_PreparedReset = desc.reset;
        m_Stats.preparedFrames++;
    }

    return result;
}

ffxReturnCode_t FrameGenerationImpl::Dispatch(ffxDispatchDescFrameGeneration* native, void* user) {
    auto& impl = *(FrameGenerationImpl*)user;
    impl.m_DispatchedThisPresent = true;
    native->backbufferTransferFunction = uint32_t(impl.m_Desc.transferFunction);
    native->minMaxLuminance[0] = impl.m_Desc.minLuminance;
    native->minMaxLuminance[1] = impl.m_Desc.maxLuminance ? impl.m_Desc.maxLuminance : (impl.m_Desc.transferFunction == FrameGenerationTransferFunction::PQ ? 1000.0f : 80.0f);
    native->reset = native->reset || impl.m_PreparedReset;
    ExclusiveScope lock(g_ffx.d3d12Lock);
    const ffxReturnCode_t result = impl.m_Dispatch(&impl.m_Context, &native->header);
    impl.m_Stats.lastDispatchResult = FfxConvertError(result);

    if (result == FFX_API_RETURN_OK) {
        impl.m_Stats.generatedDispatches++;
        impl.m_Stats.generatedFramesRecorded += native->numGeneratedFrames;
    }

    return result;
}

Result FrameGenerationImpl::Acquire(uint32_t& index) {
    if (m_Waitable) {
        const DWORD result = WaitForSingleObject(m_Waitable, OPENRHI_TIMEOUT_PRESENT);

        if (result != WAIT_OBJECT_0)
            return result == WAIT_TIMEOUT ? Result::FAILURE : GetResultFromHRESULT(HRESULT_FROM_WIN32(GetLastError()));
    }

    index = m_SwapChain->GetCurrentBackBufferIndex();

    return index < m_Textures.size() ? Result::SUCCESS : Result::FAILURE;
}

Result FrameGenerationImpl::Present(uint8_t syncInterval) {
    if (m_Stats.configuredEnabled && (!m_PreparedFrame || m_PreparedFrame != m_Stats.lastFrameId))
        return Result::INVALID_ARGUMENT;

    if (m_Stats.configuredEnabled)
        m_Stats.lastDispatchResult = Result::UNSUPPORTED;

    m_DispatchedThisPresent = false;
    const bool vsync = syncInterval != 0;
    const HRESULT hr = m_SwapChain->Present(vsync ? 1 : 0, m_AllowTearing && !vsync ? DXGI_PRESENT_ALLOW_TEARING : 0);
    m_Stats.lastPresentResult = GetResultFromHRESULT(hr);

    if (SUCCEEDED(hr))
        m_Stats.presentCalls++;

    m_PreparedFrame = 0;

    return (m_Stats.lastPresentResult == Result::SUCCESS && m_Stats.configuredEnabled && m_DispatchedThisPresent) ? m_Stats.lastDispatchResult : m_Stats.lastPresentResult;
}

Result FrameGenerationImpl::Wait() {
    ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX12 native = {};
    native.header.type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX12;
    ExclusiveScope lock(g_ffx.d3d12Lock);

    return FfxConvertError(m_Dispatch(&m_SwapContext, &native.header));
}

void FrameGenerationImpl::GetStats(FrameGenerationStats& stats) const {
    stats = m_Stats;
    m_SwapChain->GetLastPresentCount(&stats.dxgiPresentCount);
}

static Result OPENRHI_CALL CreateFrameGeneration(Device& device, const FrameGenerationDesc& desc, FrameGeneration*& context) {
    context = nullptr;
    CoreInterface core = {};
    Result result = openrhiGetInterface(device, OPENRHI_INTERFACE(CoreInterface), &core);

    if (result != Result::SUCCESS || core.GetDeviceDesc(device).graphicsAPI != GraphicsAPI::D3D12)
        return Result::UNSUPPORTED;

    auto* impl = Allocate<FrameGenerationImpl>(((DeviceBase&)device).GetAllocationCallbacks(), device, core);
    result = impl->Create(desc);

    if (result != Result::SUCCESS)
        Destroy(impl);
    else
        context = (FrameGeneration*)impl;

    return result;
}

static void OPENRHI_CALL DestroyFrameGeneration(FrameGeneration* context) {
    if (context)
        Destroy((FrameGenerationImpl*)context);
}

static Texture* const* OPENRHI_CALL GetFrameGenerationTextures(const FrameGeneration& context, uint32_t& count) {
    return ((const FrameGenerationImpl&)context).GetTextures(count);
}

static Result OPENRHI_CALL AcquireFrameGenerationTexture(FrameGeneration& context, uint32_t& index) {
    return ((FrameGenerationImpl&)context).Acquire(index);
}

static Result OPENRHI_CALL ConfigureFrameGeneration(FrameGeneration& context, const FrameGenerationConfigureDesc& desc) {
    return ((FrameGenerationImpl&)context).Configure(desc);
}

static Result OPENRHI_CALL CmdPrepareFrameGeneration(CommandBuffer& commandBuffer, FrameGeneration& context, const FrameGenerationPrepareDesc& desc) {
    return ((FrameGenerationImpl&)context).Prepare(commandBuffer, desc);
}

static Result OPENRHI_CALL PresentFrameGeneration(FrameGeneration& context, uint8_t syncInterval) {
    return ((FrameGenerationImpl&)context).Present(syncInterval);
}

static Result OPENRHI_CALL WaitFrameGeneration(FrameGeneration& context) {
    return ((FrameGenerationImpl&)context).Wait();
}

static void OPENRHI_CALL GetFrameGenerationStats(const FrameGeneration& context, FrameGenerationStats& stats) {
    ((const FrameGenerationImpl&)context).GetStats(stats);
}

static void* OPENRHI_CALL GetFrameGenerationSwapChainNativeObject(FrameGeneration& context) {
    return ((FrameGenerationImpl&)context).GetSwapChain();
}

} // namespace openrhi
#endif

Result openrhi::FillFrameGenerationInterface(FrameGenerationInterface& table) {
#if OPENRHI_ENABLE_FFX_FRAME_GENERATION
    table = {EnumerateFrameGenerationProviders, CreateFrameGeneration, DestroyFrameGeneration, GetFrameGenerationTextures, AcquireFrameGenerationTexture, ConfigureFrameGeneration, CmdPrepareFrameGeneration, PresentFrameGeneration, WaitFrameGeneration, GetFrameGenerationStats, GetFrameGenerationSwapChainNativeObject};

    return Result::SUCCESS;
#else
    MaybeUnused(table);

    return Result::UNSUPPORTED;
#endif
}
