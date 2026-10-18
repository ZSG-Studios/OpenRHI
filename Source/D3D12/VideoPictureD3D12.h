// © 2026 NVIDIA Corporation

#pragma once

namespace openrhi {

struct VideoPictureD3D12 final : public DebugNameBase {
    inline VideoPictureD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline TextureD3D12& GetTexture() const {
        return *m_Texture;
    }

    inline uint32_t GetSubresource() const {
        return m_Subresource;
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char*) OPENRHI_DEBUG_NAME_OVERRIDE {
    }

    //================================================================================================================
    // OpenRHI
    //================================================================================================================

    Result Create(const VideoPictureDesc& videoPictureDesc);

private:
    DeviceD3D12& m_Device;
    TextureD3D12* m_Texture = nullptr;
    uint32_t m_Subresource = 0;
};

} // namespace openrhi
