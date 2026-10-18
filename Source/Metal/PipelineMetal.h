// © 2026 NVIDIA Corporation

#pragma once

namespace openrhi {

struct ShaderLoadDescMetal {
    uint32_t sampleMask = ALL;
};

struct PipelineMetal final : public DebugNameBase {
    PipelineMetal(DeviceMetal& device);
    ~PipelineMetal();
    Result Create(const GraphicsPipelineDesc& desc);
    Result Create(const ComputePipelineDesc& desc);
    Result Create(const RayTracingPipelineDesc& desc);
    Result WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, uint32_t dstStride, void* dst) const;
    DeviceMetal& GetDevice() const;
    MTL::RenderPipelineState* GetRenderPipeline() const;
    MTL::ComputePipelineState* GetComputePipeline() const;
    MTL::ResourceID GetVisibleFunctionTableResourceID() const;
    MTL::ResourceID GetIntersectionFunctionTableResourceID() const;
    MTL::DepthStencilState* GetDepthStencilState() const;
    MTL::PrimitiveType GetPrimitiveType() const;
    MTL::CullMode GetCullMode() const;
    MTL::Winding GetWinding() const;
    MTL::TriangleFillMode GetFillMode() const;
    MTL::DepthClipMode GetDepthClipMode() const;
    bool IsDepthBoundsEnabled() const;
    bool HasSampleLocations() const;

    Multiview GetMultiview() const;
    uint32_t GetViewMask() const;

    const DepthBiasDesc& GetDepthBias() const;
    MTL::Size GetThreadGroupSize() const;
    MTL::Size GetMeshThreadGroupSize() const;
    MTL::Size GetTaskThreadGroupSize() const;
    void SetDebugName(const char* name) OPENRHI_DEBUG_NAME_OVERRIDE;

private:
    Result LoadFunction(const ShaderDesc& shader, MTL::Library*& library, MTL4::FunctionDescriptor*& function, const ShaderLoadDescMetal& loadDesc);

    DeviceMetal& m_Device;
    const PipelineLayoutMetal* m_Layout = nullptr;
    MTL::RenderPipelineState* m_Render = nullptr;
    MTL::ComputePipelineState* m_Compute = nullptr;
    MTL::VisibleFunctionTable* m_VisibleFunctionTable = nullptr;
    MTL::IntersectionFunctionTable* m_IntersectionFunctionTable = nullptr;
    Vector<uint8_t> m_ShaderGroupIdentifiers;
    MTL::DepthStencilState* m_DepthStencil = nullptr;
    MTL::PrimitiveType m_Primitive = MTL::PrimitiveTypeTriangle;
    MTL::CullMode m_Cull = MTL::CullModeNone;
    MTL::Winding m_Winding = MTL::WindingClockwise;
    MTL::TriangleFillMode m_Fill = MTL::TriangleFillModeFill;
    MTL::DepthClipMode m_DepthClip = MTL::DepthClipModeClip;
    bool m_DepthBounds = false;
    bool m_SampleLocations = false;
    Multiview m_Multiview = Multiview::FLEXIBLE;
    uint32_t m_ViewMask = 0;
    DepthBiasDesc m_DepthBias = {};
    MTL::Size m_ThreadGroup = MTL::Size(1, 1, 1), m_MeshGroup = MTL::Size(1, 1, 1), m_TaskGroup = MTL::Size(1, 1, 1);
    uint32_t m_MeshPayloadSize = 0;
};

} // namespace openrhi
