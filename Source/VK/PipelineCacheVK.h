// © 2026 NVIDIA Corporation

#pragma once

namespace openrhi {

struct PipelineCacheVK final : public DebugNameBase {
    inline PipelineCacheVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline operator VkPipelineCache() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~PipelineCacheVK();

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) OPENRHI_DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // OpenRHI
    //================================================================================================================

    Result Create(const PipelineCacheDesc& pipelineCacheDesc);
    Result GetData(void* dst, uint64_t& size) const;

private:
    DeviceVK& m_Device;
    VkPipelineCache m_Handle = VK_NULL_HANDLE;
};

} // namespace openrhi
