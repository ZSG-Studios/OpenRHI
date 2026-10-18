// © 2021 NVIDIA Corporation

SwapChainVal::~SwapChainVal() {
    for (size_t i = 0; i < m_Textures.size(); i++)
        Destroy(m_Textures[i]);
}

OPENRHI_INLINE Texture* const* SwapChainVal::GetTextures(uint32_t& textureNum) {
    Texture* const* textures = GetSwapChainInterfaceImpl().GetSwapChainTextures(*GetImpl(), textureNum);

    if (m_Textures.empty()) {
        for (uint32_t i = 0; i < textureNum; i++) {
            TextureVal* textureVal = Allocate<TextureVal>(m_Device.GetAllocationCallbacks(), m_Device, textures[i], true);
            m_Textures.push_back(textureVal);
        }
    }

    return (Texture* const*)m_Textures.data();
}

OPENRHI_INLINE Result SwapChainVal::AcquireNextTexture(Fence& acquireSemaphore, uint32_t& textureIndex) {
    const FenceVal& acquireSemaphoreVal = (FenceVal&)acquireSemaphore;
    OPENRHI_RETURN_ON_FAILURE(&m_Device, acquireSemaphoreVal.IsSwapChainSemaphore(), Result::INVALID_ARGUMENT, "'acquireSemaphore' must be created with 'SWAPCHAIN_SEMAPHORE' initial value");

    Fence* textureAcquiredSemaphoreImpl = OPENRHI_GET_IMPL(Fence, &acquireSemaphore);

    return GetSwapChainInterfaceImpl().AcquireNextTexture(*GetImpl(), *textureAcquiredSemaphoreImpl, textureIndex);
}

OPENRHI_INLINE Result SwapChainVal::WaitForPresent(uint64_t presentId) {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, presentId != 0, Result::INVALID_ARGUMENT, "'presentId' is 0");
    OPENRHI_RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.flags & SwapChainBits::WAITABLE, Result::FAILURE, "Swap chain has not been created with 'WAITABLE' flag");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    OPENRHI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.waitableSwapChain, Result::FAILURE, "'features.waitableSwapChain' is false");

    return GetSwapChainInterfaceImpl().WaitForPresent(*GetImpl(), presentId);
}

OPENRHI_INLINE Result SwapChainVal::Present(Fence& releaseSemaphore, uint64_t presentId) {
    const FenceVal& releaseSemaphoreVal = (FenceVal&)releaseSemaphore;
    OPENRHI_RETURN_ON_FAILURE(&m_Device, releaseSemaphoreVal.IsSwapChainSemaphore(), Result::INVALID_ARGUMENT, "'releaseSemaphore' must be created with 'SWAPCHAIN_SEMAPHORE' initial value");

    Fence* renderingFinishedSemaphoreImpl = OPENRHI_GET_IMPL(Fence, &releaseSemaphore);

    return GetSwapChainInterfaceImpl().QueuePresent(*GetImpl(), *renderingFinishedSemaphoreImpl, presentId);
}

OPENRHI_INLINE Result SwapChainVal::GetDisplayDesc(DisplayDesc& displayDesc) const {
    return GetSwapChainInterfaceImpl().GetDisplayDesc(*GetImpl(), displayDesc);
}

OPENRHI_INLINE Result SwapChainVal::SetLatencySleepMode(const LatencySleepMode& latencySleepMode) {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.flags & SwapChainBits::ALLOW_LOW_LATENCY, Result::FAILURE, "Swap chain has not been created with 'ALLOW_LOW_LATENCY' flag");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    OPENRHI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.lowLatency, Result::FAILURE, "'features.lowLatency' is false");

    return GetLowLatencyInterfaceImpl().SetLatencySleepMode(*GetImpl(), latencySleepMode);
}

OPENRHI_INLINE Result SwapChainVal::SetLatencyMarker(uint64_t presentId, LatencyMarker latencyMarker) {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, presentId != 0, Result::INVALID_ARGUMENT, "'presentId' is 0");
    OPENRHI_RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.flags & SwapChainBits::ALLOW_LOW_LATENCY, Result::FAILURE, "Swap chain has not been created with 'ALLOW_LOW_LATENCY' flag");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    OPENRHI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.lowLatency, Result::FAILURE, "'features.lowLatency' is false");

    return GetLowLatencyInterfaceImpl().SetLatencyMarker(*GetImpl(), presentId, latencyMarker);
}

OPENRHI_INLINE Result SwapChainVal::LatencySleep(uint64_t presentId) {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, presentId != 0, Result::INVALID_ARGUMENT, "'presentId' is 0");
    OPENRHI_RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.flags & SwapChainBits::ALLOW_LOW_LATENCY, Result::FAILURE, "Swap chain has not been created with 'ALLOW_LOW_LATENCY' flag");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    OPENRHI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.lowLatency, Result::FAILURE, "'features.lowLatency' is false");

    return GetLowLatencyInterfaceImpl().LatencySleep(*GetImpl(), presentId);
}

OPENRHI_INLINE Result SwapChainVal::GetLatencyReport(LatencyReport& latencyReport) {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.flags & SwapChainBits::ALLOW_LOW_LATENCY, Result::FAILURE, "Swap chain has not been created with 'ALLOW_LOW_LATENCY' flag");

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    OPENRHI_RETURN_ON_FAILURE(&m_Device, deviceDesc.features.lowLatency, Result::FAILURE, "'features.lowLatency' is false");

    return GetLowLatencyInterfaceImpl().GetLatencyReport(*GetImpl(), latencyReport);
}
