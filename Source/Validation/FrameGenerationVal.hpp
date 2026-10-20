// Frame generation validation uses the public facade; native objects remain owned by the provider
#if OPENRHI_ENABLE_FFX_FRAME_GENERATION
struct FrameGenerationVal final : public ObjectVal {
    FrameGenerationVal(DeviceVal& device, FrameGeneration* impl, const FrameGenerationDesc& desc)
        : ObjectVal(device, impl), m_Desc(desc) {
    }

    FrameGeneration* GetImpl() const {
        return (FrameGeneration*)m_Impl;
    }

    const FrameGenerationDesc& GetDesc() const {
        return m_Desc;
    }

private:
    FrameGenerationDesc m_Desc;
};

static inline FrameGenerationInterface GetFrameGenerationTable() {
    FrameGenerationInterface table = {};
    FillFrameGenerationInterface(table);

    return table;
}

static Result OPENRHI_CALL EnumerateFrameGenerationProvidersVal(const Device& device, FrameGenerationProvider* providers, uint32_t& providerNum) {
    auto& val = (DeviceVal&)device;
    OPENRHI_RETURN_ON_FAILURE(&val, !providers || providerNum, Result::INVALID_ARGUMENT, "Frame generation provider capacity is zero");

    return GetFrameGenerationTable().EnumerateFrameGenerationProviders(device, providers, providerNum);
}

static Result OPENRHI_CALL CreateFrameGenerationVal(Device& device, const FrameGenerationDesc& desc, FrameGeneration*& context) {
    auto& val = (DeviceVal&)device;
    context = nullptr;
    const auto& swap = desc.swapChain;
    OPENRHI_RETURN_ON_FAILURE(&val, swap.queue && swap.window.windows.hwnd, Result::INVALID_ARGUMENT, "Frame generation queue or HWND is null");
    OPENRHI_RETURN_ON_FAILURE(&val, ((const QueueVal*)swap.queue)->GetType() == QueueType::GRAPHICS, Result::INVALID_ARGUMENT, "Frame generation requires a graphics queue");
    OPENRHI_RETURN_ON_FAILURE(&val, swap.width && swap.height && swap.textureNum >= 2 && swap.textureNum <= 16 && swap.format < SwapChainFormat::MAX_NUM, Result::INVALID_ARGUMENT, "Frame generation swapchain dimensions, count or format is invalid");
    OPENRHI_RETURN_ON_FAILURE(&val, desc.maxRenderResolution.w && desc.maxRenderResolution.h, Result::INVALID_ARGUMENT, "Frame generation render dimensions are zero");
    OPENRHI_RETURN_ON_FAILURE(&val, desc.transferFunction < FrameGenerationTransferFunction::MAX_NUM, Result::INVALID_ARGUMENT, "Frame generation transfer function is invalid");
    OPENRHI_RETURN_ON_FAILURE(&val, std::isfinite(desc.minLuminance) && std::isfinite(desc.maxLuminance) && desc.minLuminance >= 0 && (desc.maxLuminance == 0 || desc.maxLuminance > desc.minLuminance), Result::INVALID_ARGUMENT, "Frame generation luminance bounds are invalid");
    constexpr auto allowed = UpscalerBits::HDR | UpscalerBits::DEPTH_INVERTED | UpscalerBits::DEPTH_INFINITE | UpscalerBits::MV_UPSCALED | UpscalerBits::MV_JITTERED;
    OPENRHI_RETURN_ON_FAILURE(&val, (desc.flags & ~allowed) == 0, Result::INVALID_ARGUMENT, "Frame generation flags are invalid");
    OPENRHI_RETURN_ON_FAILURE(&val, (swap.flags & ~(SwapChainBits::VSYNC | SwapChainBits::WAITABLE | SwapChainBits::ALLOW_TEARING)) == 0, Result::INVALID_ARGUMENT, "Frame generation swapchain flags are invalid");
    FrameGeneration* impl = nullptr;
    Result result = GetFrameGenerationTable().CreateFrameGeneration(device, desc, impl);

    if (result == Result::SUCCESS)
        context = (FrameGeneration*)Allocate<FrameGenerationVal>(val.GetAllocationCallbacks(), val, impl, desc);

    return result;
}

static void OPENRHI_CALL DestroyFrameGenerationVal(FrameGeneration* context) {
    if (!context)
        return;

    auto* val = (FrameGenerationVal*)context;
    GetFrameGenerationTable().DestroyFrameGeneration(val->GetImpl());
    Destroy(val);
}

static Texture* const* OPENRHI_CALL GetFrameGenerationTexturesVal(const FrameGeneration& context, uint32_t& count) {
    return GetFrameGenerationTable().GetFrameGenerationTextures(*((const FrameGenerationVal&)context).GetImpl(), count);
}

static Result OPENRHI_CALL AcquireFrameGenerationTextureVal(FrameGeneration& context, uint32_t& index) {
    return GetFrameGenerationTable().AcquireFrameGenerationTexture(*((FrameGenerationVal&)context).GetImpl(), index);
}

static inline bool ValidateFrameGenerationTexture(DeviceVal& device, const Texture* texture, uint32_t width, uint32_t height, uint32_t maxWidth = 0, uint32_t maxHeight = 0) {
    if (!texture)
        return true;

    const auto& desc = ((const TextureVal*)texture)->GetDesc();
    OPENRHI_RETURN_ON_FAILURE(&device, desc.type == TextureType::TEXTURE_2D && desc.width >= width && desc.width <= (maxWidth ? maxWidth : width) && desc.height >= height && desc.height <= (maxHeight ? maxHeight : height) && desc.depth == 1 && desc.mipNum == 1 && desc.layerNum == 1 && desc.sampleNum == 1, false, "Frame generation texture shape is invalid");
    OPENRHI_RETURN_ON_FAILURE(&device, (desc.usage & TextureUsageBits::SHADER_RESOURCE) != 0, false, "Frame generation texture needs SHADER_RESOURCE usage");

    return true;
}

static Result OPENRHI_CALL ConfigureFrameGenerationVal(FrameGeneration& context, const FrameGenerationConfigureDesc& desc) {
    auto& val = (FrameGenerationVal&)context;
    const auto& creation = val.GetDesc();
    OPENRHI_RETURN_ON_FAILURE(&val.GetDevice(), !desc.enabled || desc.frameId, Result::INVALID_ARGUMENT, "Enabled frame generation requires nonzero frameId");

    if (!ValidateFrameGenerationTexture(val.GetDevice(), desc.hudlessColor, creation.swapChain.width, creation.swapChain.height) || !ValidateFrameGenerationTexture(val.GetDevice(), desc.ui, creation.swapChain.width, creation.swapChain.height))
        return Result::INVALID_ARGUMENT;

    if (desc.ui) {
        const Format format = ((const TextureVal*)desc.ui)->GetDesc().format;
        OPENRHI_RETURN_ON_FAILURE(&val.GetDevice(), format == Format::RGBA8_UNORM || format == Format::RGBA16_SFLOAT, Result::INVALID_ARGUMENT, "Frame generation UI format must be RGBA8_UNORM or RGBA16_SFLOAT");
    }

    if (desc.hudlessColor) {
        constexpr Format formats[] = {Format::RGBA16_SFLOAT, Format::RGBA8_UNORM, Format::R10_G10_B10_A2_UNORM, Format::R10_G10_B10_A2_UNORM};
        OPENRHI_RETURN_ON_FAILURE(&val.GetDevice(), ((TextureVal*)desc.hudlessColor)->GetDesc().format == formats[uint32_t(creation.swapChain.format)], Result::INVALID_ARGUMENT, "Frame generation HUDless format must match swapchain");
    }

    return GetFrameGenerationTable().ConfigureFrameGeneration(*val.GetImpl(), desc);
}

static Result OPENRHI_CALL CmdPrepareFrameGenerationVal(CommandBuffer& commandBuffer, FrameGeneration& context, const FrameGenerationPrepareDesc& desc) {
    auto& val = (FrameGenerationVal&)context;
    auto& device = val.GetDevice();
    const auto& creation = val.GetDesc();
    OPENRHI_RETURN_ON_FAILURE(&device, ((CommandBufferVal&)commandBuffer).CanDispatch(), Result::INVALID_ARGUMENT, "Frame generation preparation requires an open compute-capable command buffer outside rendering");
    OPENRHI_RETURN_ON_FAILURE(&device, desc.frameId && desc.depth && desc.motionVectors, Result::INVALID_ARGUMENT, "Frame generation frameId or guide textures are invalid");
    OPENRHI_RETURN_ON_FAILURE(&device, desc.renderResolution.w && desc.renderResolution.h && desc.renderResolution.w <= creation.maxRenderResolution.w && desc.renderResolution.h <= creation.maxRenderResolution.h, Result::INVALID_ARGUMENT, "Frame generation render resolution is invalid");
    const bool displayMotion = (creation.flags & UpscalerBits::MV_UPSCALED) != 0;
    if (!ValidateFrameGenerationTexture(device, desc.depth, desc.renderResolution.w, desc.renderResolution.h, creation.maxRenderResolution.w, creation.maxRenderResolution.h) || !ValidateFrameGenerationTexture(device, desc.motionVectors, displayMotion ? creation.swapChain.width : desc.renderResolution.w, displayMotion ? creation.swapChain.height : desc.renderResolution.h, displayMotion ? 0 : creation.maxRenderResolution.w, displayMotion ? 0 : creation.maxRenderResolution.h))
        return Result::INVALID_ARGUMENT;

    OPENRHI_RETURN_ON_FAILURE(&device, ((const TextureVal*)desc.depth)->GetDesc().format == Format::R32_SFLOAT, Result::INVALID_ARGUMENT, "Frame generation depth format must be R32_SFLOAT");
    const Format motionFormat = ((const TextureVal*)desc.motionVectors)->GetDesc().format;
    OPENRHI_RETURN_ON_FAILURE(&device, motionFormat == Format::RG16_SFLOAT || motionFormat == Format::RG32_SFLOAT, Result::INVALID_ARGUMENT, "Frame generation motion format must be RG16_SFLOAT or RG32_SFLOAT");

    OPENRHI_RETURN_ON_FAILURE(&device, std::isfinite(desc.frameTimeMs) && desc.frameTimeMs > 0 && std::isfinite(desc.cameraNear) && desc.cameraNear > 0 && std::isfinite(desc.cameraFar) && ((creation.flags & UpscalerBits::DEPTH_INFINITE) != 0 || desc.cameraFar > desc.cameraNear) && std::isfinite(desc.verticalFov) && desc.verticalFov > 0 && desc.verticalFov < 3.141593f && std::isfinite(desc.viewSpaceToMetersFactor) && desc.viewSpaceToMetersFactor > 0, Result::INVALID_ARGUMENT, "Frame generation camera parameters are invalid");
    OPENRHI_RETURN_ON_FAILURE(&device, std::isfinite(desc.cameraJitter.x) && std::isfinite(desc.cameraJitter.y) && std::isfinite(desc.motionVectorScale.x) && std::isfinite(desc.motionVectorScale.y), Result::INVALID_ARGUMENT, "Frame generation jitter or motion scale is not finite");

    for (uint32_t i = 0; i < 3; i++)
        OPENRHI_RETURN_ON_FAILURE(&device, std::isfinite(desc.cameraPosition[i]) && std::isfinite(desc.cameraUp[i]) && std::isfinite(desc.cameraRight[i]) && std::isfinite(desc.cameraForward[i]), Result::INVALID_ARGUMENT, "Frame generation camera basis is not finite");

    return GetFrameGenerationTable().CmdPrepareFrameGeneration(commandBuffer, *val.GetImpl(), desc);
}

static Result OPENRHI_CALL PresentFrameGenerationVal(FrameGeneration& context, uint8_t syncInterval) {
    auto& val = (FrameGenerationVal&)context;
    OPENRHI_RETURN_ON_FAILURE(&val.GetDevice(), syncInterval <= 1, Result::INVALID_ARGUMENT, "Frame generation syncInterval must be 0 or 1");

    return GetFrameGenerationTable().PresentFrameGeneration(*val.GetImpl(), syncInterval);
}

static Result OPENRHI_CALL WaitFrameGenerationVal(FrameGeneration& context) {
    return GetFrameGenerationTable().WaitFrameGeneration(*((FrameGenerationVal&)context).GetImpl());
}

static void OPENRHI_CALL GetFrameGenerationStatsVal(const FrameGeneration& context, FrameGenerationStats& stats) {
    GetFrameGenerationTable().GetFrameGenerationStats(*((const FrameGenerationVal&)context).GetImpl(), stats);
}

static void* OPENRHI_CALL GetFrameGenerationSwapChainNativeObjectVal(FrameGeneration& context) {
    return GetFrameGenerationTable().GetFrameGenerationSwapChainNativeObject(*((FrameGenerationVal&)context).GetImpl());
}
#endif

Result DeviceVal::FillFunctionTable(FrameGenerationInterface& table) const {
#if OPENRHI_ENABLE_FFX_FRAME_GENERATION
    if (GetDesc().graphicsAPI != GraphicsAPI::D3D12)
        return Result::UNSUPPORTED;

    table = {EnumerateFrameGenerationProvidersVal, CreateFrameGenerationVal, DestroyFrameGenerationVal, GetFrameGenerationTexturesVal, AcquireFrameGenerationTextureVal, ConfigureFrameGenerationVal, CmdPrepareFrameGenerationVal, PresentFrameGenerationVal, WaitFrameGenerationVal, GetFrameGenerationStatsVal, GetFrameGenerationSwapChainNativeObjectVal};

    return Result::SUCCESS;
#else
    MaybeUnused(table);

    return Result::UNSUPPORTED;
#endif
}
