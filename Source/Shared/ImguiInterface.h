// © 2025 NVIDIA Corporation

#pragma once

#if OPENRHI_ENABLE_IMGUI_EXTENSION

namespace openrhi {

struct ImguiPipeline {
    Pipeline* pipeline = nullptr;
    Format format = Format::UNKNOWN;
    bool writeAlpha = false;
};

struct ImguiTexture {
    Texture* texture = nullptr;
    Descriptor* descriptor = nullptr;
    uint64_t updateTick = 0;
};

struct ImguiImpl final : public DebugNameBase {
    inline ImguiImpl(Device& device, const CoreInterface& OpenRHI)
        : m_Device(device)
        , m_iCore(OpenRHI)
        , m_Textures(((DeviceBase&)device).GetStdAllocator())
        , m_Pipelines(((DeviceBase&)device).GetStdAllocator())
        , m_DescriptorSets1(((DeviceBase&)device).GetStdAllocator()) {
    }

    ~ImguiImpl();

    inline Device& GetDevice() {
        return m_Device;
    }

    Result Create(const ImguiDesc& imguiDesc);
    void CmdCopyData(CommandBuffer& commandBuffer, Streamer& streamer, const CopyImguiDataDesc& copyImguiDataDesc, ImguiRenderData& imguiRenderData);
    void CmdDraw(CommandBuffer& commandBuffer, const ImguiRenderData& imguiRenderData, const DrawImguiDesc& drawImguiDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) OPENRHI_DEBUG_NAME_OVERRIDE {
        m_iCore.SetDebugName(m_Sampler, name);
        m_iCore.SetDebugName(m_DescriptorPool, name);
        m_iCore.SetDebugName(m_PipelineLayout, name);
    }

private:
    void CmdCopyDataInternal(CommandBuffer& commandBuffer, Streamer& streamer, const CopyImguiDataDesc& copyImguiDataDesc, ImguiRenderData& imguiRenderData);
    void CmdDrawInternal(CommandBuffer& commandBuffer, const ImguiRenderData& imguiRenderData, const DrawImguiDesc& drawImguiDesc);

    Device& m_Device;
    const CoreInterface& m_iCore;
    StreamerInterface m_iStreamer = {};
    UnorderedMap<uint64_t, ImguiTexture> m_Textures;
    Vector<ImguiPipeline> m_Pipelines;
    Vector<DescriptorSet*> m_DescriptorSets1;
    Descriptor* m_Sampler = nullptr;
    DescriptorPool* m_DescriptorPool = nullptr;
    PipelineLayout* m_PipelineLayout = nullptr;
    DescriptorSet* m_DescriptorSet0_sampler = nullptr;
    uint32_t m_DescriptorSetIndex = 0;
    Lock m_Lock;
    static Lock s_TextureLock;
};

} // namespace openrhi

#endif
