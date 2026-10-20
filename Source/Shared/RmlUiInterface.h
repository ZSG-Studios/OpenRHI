// OpenRHI RmlUi rendering implementation
#pragma once

namespace openrhi {

struct RmlUiImpl final : public DebugNameBase {
    RmlUiImpl(Device& device, const CoreInterface& core)
        : m_Device(device)
        , m_Core(core)
        , m_TextureSets(((DeviceBase&)device).GetStdAllocator()) {
    }

    ~RmlUiImpl();
    Result Create(const RmlUiDesc& desc);
    Result CmdDraw(CommandBuffer& commandBuffer, Dim2_t displaySize, const DrawRmlUiDesc* draws, uint32_t drawNum);

    void Reset() {
        m_DrawIndex = 0;
    }

    Device& GetDevice() {
        return m_Device;
    }

    void SetDebugName(const char* name) OPENRHI_DEBUG_NAME_OVERRIDE {
        m_Core.SetDebugName(m_Sampler, name);
        m_Core.SetDebugName(m_Pool, name);
        m_Core.SetDebugName(m_Layout, name);
        m_Core.SetDebugName(m_Pipeline, name);
    }

private:
    Device& m_Device;
    CoreInterface m_Core;
    Vector<DescriptorSet*> m_TextureSets;
    Descriptor* m_Sampler = nullptr;
    DescriptorPool* m_Pool = nullptr;
    DescriptorSet* m_SamplerSet = nullptr;
    PipelineLayout* m_Layout = nullptr;
    Pipeline* m_Pipeline = nullptr;
    uint32_t m_DrawIndex = 0;
    bool m_LinearColor = false;
    float m_HdrScale = 1;
};

Result FillRmlUiInterface(RmlUiInterface& table);

} // namespace openrhi
