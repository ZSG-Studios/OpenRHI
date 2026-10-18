// © 2026 NVIDIA Corporation
#pragma once

namespace openrhi {

struct PipelineLayoutMetal final : public DebugNameBase {
    PipelineLayoutMetal(DeviceMetal& device);
    ~PipelineLayoutMetal();
    Result Create(const PipelineLayoutDesc& desc);
    DeviceMetal& GetDevice() const;
    uint32_t GetRootDataSize() const;
    uint32_t GetRootConstantOffset(uint32_t index) const;
    uint32_t GetRootDescriptorOffset(uint32_t index) const;
    uint32_t GetDrawParametersOffset() const;
    uint32_t GetDrawIndexOffset() const;
    bool IsDrawParametersEmulationEnabled() const;
    bool IsDrawIndexEmulationEnabled() const;
    const DescriptorSetMappingMetal& GetDescriptorSetMapping(uint32_t index) const;
    void InitRootData(void* data) const;
    void WriteSetPointers(void* data, uint32_t setIndex, const DescriptorSetMetal& set) const;

    void SetDebugName(const char*) OPENRHI_DEBUG_NAME_OVERRIDE {
    }

private:
    DeviceMetal& m_Device;
    Vector<DescriptorSetMappingMetal> m_Sets;
    Vector<uint32_t> m_ConstantOffsets;
    Vector<uint32_t> m_DescriptorOffsets;
    Vector<uint32_t> m_SetOffsets;
    Vector<DescriptorMetal*> m_RootSamplers;
    MTL::Buffer* m_RootSamplerBuffer = nullptr;
    uint32_t m_RootSamplerOffset = UINT32_MAX;
    uint32_t m_DrawParametersOffset = UINT32_MAX;
    uint32_t m_DrawIndexOffset = UINT32_MAX;
    uint32_t m_RootDataSize = 0;
};

} // namespace openrhi
