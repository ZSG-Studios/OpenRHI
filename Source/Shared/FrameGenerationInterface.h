// OpenRHI AMD native frame generation provider
#pragma once

#if OPENRHI_ENABLE_FFX_FRAME_GENERATION
#    include "ffx_framegeneration.h"
#    include "dx12/ffx_api_framegeneration_dx12.h"

namespace openrhi {

struct FrameGenerationImpl final : public DebugNameBase {
    FrameGenerationImpl(Device& device, const CoreInterface& core)
        : m_Device(device), m_Core(core), m_Textures(((DeviceBase&)device).GetStdAllocator()) {
        m_Stats.lastDispatchResult = Result::UNSUPPORTED;
        m_Stats.lastPresentResult = Result::UNSUPPORTED;
    }
    ~FrameGenerationImpl();
    Device& GetDevice() {
        return m_Device;
    }
    Result Create(const FrameGenerationDesc& desc);
    Result Configure(const FrameGenerationConfigureDesc& desc);
    Result Prepare(CommandBuffer& commandBuffer, const FrameGenerationPrepareDesc& desc);
    Result Acquire(uint32_t& index);
    Result Present(uint8_t syncInterval);
    Result Wait();
    void GetStats(FrameGenerationStats& stats) const;
    Texture* const* GetTextures(uint32_t& count) const {
        count = uint32_t(m_Textures.size());

        return m_Textures.data();
    }
    void* GetSwapChain() {
        return m_SwapChain;
    }
    const FrameGenerationDesc& GetDesc() const {
        return m_Desc;
    }

private:
    static ffxReturnCode_t Dispatch(ffxDispatchDescFrameGeneration* native, void* user);
    Device& m_Device;
    CoreInterface m_Core;
    Vector<Texture*> m_Textures;
    FrameGenerationDesc m_Desc = {};
    FrameGenerationStats m_Stats = {};
    Library* m_Library = nullptr;
    PfnFfxCreateContext m_Create = nullptr;
    PfnFfxDestroyContext m_Destroy = nullptr;
    PfnFfxConfigure m_Configure = nullptr;
    PfnFfxQuery m_Query = nullptr;
    PfnFfxDispatch m_Dispatch = nullptr;
    ffxContext m_Context = nullptr;
    ffxContext m_SwapContext = nullptr;
    IDXGIFactory2* m_Factory = nullptr;
    IDXGISwapChain4* m_SwapChain = nullptr;
    HANDLE m_Waitable = nullptr;
    uint64_t m_PreparedFrame = 0;
    bool m_PreparedReset = false;
    bool m_DispatchedThisPresent = false;
    bool m_AllowTearing = false;
    ffxCreateBackendDX12Desc m_Backend = {};
    ffxCreateContextDescFrameGeneration m_ContextDesc = {};
    ffxCreateContextDescFrameGenerationVersion m_Version = {};
    ffxOverrideVersion m_Override = {};
    ffxCreateContextDescFrameGenerationSwapChainForHwndDX12 m_SwapDesc = {};
    ffxCreateContextDescFrameGenerationSwapChainVersionDX12 m_SwapVersion = {};
    DXGI_SWAP_CHAIN_DESC1 m_DxgiDesc = {};
    ffxConfigureDescFrameGeneration m_Config = {};
    ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12 m_UiConfig = {};
};

} // namespace openrhi
#endif
