// © 2021 NVIDIA Corporation

#pragma once

namespace openrhi {

struct FenceVK final : public DebugNameBase {
    inline FenceVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline operator VkSemaphore() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~FenceVK();

    Result Create(uint64_t initialValue);
    Result Create(const FenceVKDesc& fenceVKDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) OPENRHI_DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // OpenRHI
    //================================================================================================================

    uint64_t GetFenceValue() const;
    Result Wait(uint64_t value);

private:
    DeviceVK& m_Device;
    VkSemaphore m_Handle = VK_NULL_HANDLE;
    bool m_OwnsNativeObjects = true;
};

} // namespace openrhi
