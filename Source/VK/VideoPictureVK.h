// © 2026 NVIDIA Corporation

#pragma once

namespace openrhi {

struct VideoPictureVK final : public DebugNameBase {
    inline VideoPictureVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline bool IsSameSubresource(const VideoPictureVK& picture) const {
        return m_Texture == picture.m_Texture && m_Layer == picture.m_Layer;
    }

    inline const VkVideoPictureResourceInfoKHR& GetResource() const {
        return m_Resource;
    }

    ~VideoPictureVK();

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) OPENRHI_DEBUG_NAME_OVERRIDE {
        m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_ImageView, name);
    }

    //================================================================================================================
    // OpenRHI
    //================================================================================================================

    Result Create(const VideoPictureDesc& videoPictureDesc);

private:
    DeviceVK& m_Device;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkVideoPictureResourceInfoKHR m_Resource = {};
    TextureVK* m_Texture = nullptr;
    uint32_t m_Layer = 0;
};

} // namespace openrhi
