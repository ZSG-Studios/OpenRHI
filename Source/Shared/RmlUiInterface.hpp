// OpenRHI RmlUi renderer, shared by native backends and Validation
#if OPENRHI_ENABLE_RMLUI_EXTENSION
#    include "RmlUi_fs.h"
#    include "RmlUi_vs.h"

namespace openrhi {

struct RmlUiConstants {
    float transform[16];
    float invDisplay[2];
    float translation[2];
    float hdrScale;
    uint32_t linearColor;
    float padding[2];
};

static_assert(sizeof(RmlUiConstants) == 96, "RmlUi root constants ABI");
static_assert(sizeof(RmlUiVertex) == 20, "RmlUi vertex ABI");

RmlUiImpl::~RmlUiImpl() {
    m_Core.DestroyPipeline(m_Pipeline);
    m_Core.DestroyDescriptorPool(m_Pool);
    m_Core.DestroyPipelineLayout(m_Layout);
    m_Core.DestroyDescriptor(m_Sampler);
}

Result RmlUiImpl::Create(const RmlUiDesc& desc) {
    m_LinearColor = desc.linearColor;
    m_HdrScale = desc.hdrScale == 0 ? 1 : desc.hdrScale;
    m_TextureSets.resize(desc.maxDrawNum);

    SamplerDesc sampler = {};
    sampler.filters.min = Filter::LINEAR;
    sampler.filters.mag = Filter::LINEAR;
    sampler.addressModes.u = AddressMode::CLAMP_TO_EDGE;
    sampler.addressModes.v = AddressMode::CLAMP_TO_EDGE;
    sampler.addressModes.w = AddressMode::CLAMP_TO_EDGE;
    Result result = m_Core.CreateSampler(m_Device, sampler, m_Sampler);

    if (result != Result::SUCCESS)
        return result;

    RootConstantDesc constants = {};
    constants.registerIndex = 0;
    constants.shaderStages = StageBits::VERTEX_SHADER | StageBits::FRAGMENT_SHADER;
    constants.size = sizeof(RmlUiConstants);
    DescriptorRangeDesc ranges[] = {
        {1, 1, DescriptorType::SAMPLER, StageBits::FRAGMENT_SHADER},
        {0, 1, DescriptorType::TEXTURE, StageBits::FRAGMENT_SHADER},
    };
    DescriptorSetDesc sets[2] = {};

    for (uint32_t i = 0; i < 2; i++) {
        sets[i].registerSpace = i;
        sets[i].ranges = &ranges[i];
        sets[i].rangeNum = 1;
    }

    PipelineLayoutDesc layout = {};
    layout.rootConstants = &constants;
    layout.rootConstantNum = 1;
    layout.descriptorSets = sets;
    layout.descriptorSetNum = 2;
    layout.shaderStages = StageBits::VERTEX_SHADER | StageBits::FRAGMENT_SHADER;
    layout.flags = PipelineLayoutBits::IGNORE_GLOBAL_SPIRV_OFFSETS;
    result = m_Core.CreatePipelineLayout(m_Device, layout, m_Layout);

    if (result != Result::SUCCESS)
        return result;

    DescriptorPoolDesc pool = {};
    pool.descriptorSetMaxNum = desc.maxDrawNum + 1;
    pool.textureMaxNum = desc.maxDrawNum;
    pool.samplerMaxNum = 1;
    result = m_Core.CreateDescriptorPool(m_Device, pool, m_Pool);

    if (result != Result::SUCCESS)
        return result;

    result = m_Core.AllocateDescriptorSets(*m_Pool, *m_Layout, 0, &m_SamplerSet, 1, 0);

    if (result != Result::SUCCESS)
        return result;

    result = m_Core.AllocateDescriptorSets(*m_Pool, *m_Layout, 1, m_TextureSets.data(), desc.maxDrawNum, 0);

    if (result != Result::SUCCESS)
        return result;

    UpdateDescriptorRangeDesc update = {m_SamplerSet, 0, 0, &m_Sampler, 1};
    m_Core.UpdateDescriptorRanges(&update, 1);

    const VertexAttributeDesc attributes[] = {
        {{"POSITION", 0}, {0}, offsetof(RmlUiVertex, position), Format::RG32_SFLOAT},
        {{"COLOR", 0}, {1}, offsetof(RmlUiVertex, color), Format::RGBA8_UNORM},
        {{"TEXCOORD", 0}, {2}, offsetof(RmlUiVertex, uv), Format::RG32_SFLOAT},
    };
    VertexStreamDesc stream = {};
    stream.stride = sizeof(RmlUiVertex);
    VertexInputDesc input = {};
    input.attributes = attributes;
    input.attributeNum = uint8_t(GetCountOf(attributes));
    input.streams = &stream;
    input.streamNum = 1;
    ColorAttachmentDesc color = {};
    color.format = desc.attachmentFormat;
    color.colorBlend = {BlendFactor::ONE, BlendFactor::ONE_MINUS_SRC_ALPHA, BlendOp::ADD};
    color.alphaBlend = color.colorBlend;
    color.colorWriteMask = ColorWriteBits::RGBA;
    color.blendEnabled = true;
    ShaderDesc shaders[] = {g_RmlUi_vs_desc, g_RmlUi_fs_desc};
    GraphicsPipelineDesc pipeline = {};
    pipeline.pipelineLayout = m_Layout;
    pipeline.vertexInput = &input;
    pipeline.inputAssembly.topology = Topology::TRIANGLE_LIST;
    pipeline.rasterization.fillMode = FillMode::SOLID;
    pipeline.rasterization.cullMode = CullMode::NONE;
    pipeline.outputMerger.colors = &color;
    pipeline.outputMerger.colorNum = 1;
    pipeline.shaders = shaders;
    pipeline.shaderNum = GetCountOf(shaders);

    return m_Core.CreateGraphicsPipeline(m_Device, pipeline, m_Pipeline);
}

Result RmlUiImpl::CmdDraw(CommandBuffer& commandBuffer, Dim2_t displaySize, const DrawRmlUiDesc* draws, uint32_t drawNum) {
    if (drawNum > (m_TextureSets.size() - m_DrawIndex))
        return Result::OUT_OF_MEMORY;

    if (!drawNum)
        return Result::SUCCESS;

    m_Core.CmdSetDescriptorPool(commandBuffer, *m_Pool);
    m_Core.CmdSetPipelineLayout(commandBuffer, BindPoint::GRAPHICS, *m_Layout);
    m_Core.CmdSetPipeline(commandBuffer, *m_Pipeline);
    SetDescriptorSetDesc samplerSet = {0, m_SamplerSet};
    m_Core.CmdSetDescriptorSet(commandBuffer, samplerSet);
    Viewport viewport = {};
    viewport.width = displaySize.w;
    viewport.height = displaySize.h;
    viewport.depthMax = 1;
    m_Core.CmdSetViewports(commandBuffer, &viewport, 1);

    for (uint32_t i = 0; i < drawNum; i++) {
        const DrawRmlUiDesc& draw = draws[i];

        if (!draw.scissor.width || !draw.scissor.height || !draw.geometry.indexNum)
            continue;

        DescriptorSet* set = m_TextureSets[m_DrawIndex++];
        UpdateDescriptorRangeDesc update = {set, 0, 0, &draw.texture, 1};
        m_Core.UpdateDescriptorRanges(&update, 1);
        SetDescriptorSetDesc binding = {1, set};
        m_Core.CmdSetDescriptorSet(commandBuffer, binding);
        RmlUiConstants constants = {};
        memcpy(constants.transform, draw.transform, sizeof(constants.transform));
        memcpy(constants.translation, draw.translation, sizeof(constants.translation));
        constants.invDisplay[0] = 1.0f / displaySize.w;
        constants.invDisplay[1] = 1.0f / displaySize.h;
        constants.hdrScale = m_HdrScale;
        constants.linearColor = m_LinearColor;
        SetRootConstantsDesc root = {0, &constants, sizeof(constants)};
        m_Core.CmdSetRootConstants(commandBuffer, root);
        VertexBufferDesc vertex = {};
        vertex.buffer = draw.geometry.vertices.buffer;
        vertex.offset = draw.geometry.vertices.offset;
        vertex.stride = sizeof(RmlUiVertex);
        m_Core.CmdSetVertexBuffers(commandBuffer, 0, &vertex, 1);
        m_Core.CmdSetIndexBuffer(commandBuffer, *draw.geometry.indices.buffer, draw.geometry.indices.offset, IndexType::UINT32);
        m_Core.CmdSetScissors(commandBuffer, &draw.scissor, 1);
        DrawIndexedDesc indexed = {};
        indexed.indexNum = draw.geometry.indexNum;
        indexed.instanceNum = 1;
        m_Core.CmdDrawIndexed(commandBuffer, indexed);
    }

    return Result::SUCCESS;
}

static RmlUiCapabilities OPENRHI_CALL GetRmlUiCapabilities(const Device& device) {
    const bool native = ((const DeviceBase&)device).GetDesc().graphicsAPI != GraphicsAPI::NONE;

    return {native, true, true, true, true, false, false, false, false};
}

static Result OPENRHI_CALL CreateRmlUi(Device& device, const RmlUiDesc& desc, RmlUi*& rmlUi) {
    rmlUi = nullptr;
    CoreInterface core = {};
    Result result = openrhiGetInterface(device, OPENRHI_INTERFACE(CoreInterface), &core);

    if (result != Result::SUCCESS)
        return result;

    RmlUiImpl* impl = Allocate<RmlUiImpl>(((DeviceBase&)device).GetAllocationCallbacks(), device, core);
    result = impl->Create(desc);

    if (result != Result::SUCCESS)
        Destroy(impl);
    else
        rmlUi = (RmlUi*)impl;

    return result;
}

static void OPENRHI_CALL DestroyRmlUi(RmlUi* rmlUi) {
    if (rmlUi)
        Destroy((RmlUiImpl*)rmlUi);
}

static void OPENRHI_CALL ResetRmlUi(RmlUi& rmlUi) {
    ((RmlUiImpl&)rmlUi).Reset();
}

static Result OPENRHI_CALL CmdDrawRmlUi(CommandBuffer& commandBuffer, RmlUi& rmlUi, Dim2_t displaySize, const DrawRmlUiDesc* draws, uint32_t drawNum) {
    return ((RmlUiImpl&)rmlUi).CmdDraw(commandBuffer, displaySize, draws, drawNum);
}

Result FillRmlUiInterface(RmlUiInterface& table) {
    table = {GetRmlUiCapabilities, CreateRmlUi, DestroyRmlUi, ResetRmlUi, CmdDrawRmlUi};

    return Result::SUCCESS;
}

} // namespace openrhi
#endif

Result openrhi::DeviceBase::FillFunctionTable(RmlUiInterface& table) const {
#if OPENRHI_ENABLE_RMLUI_EXTENSION
    return FillRmlUiInterface(table);
#else
    MaybeUnused(table);

    return Result::UNSUPPORTED;
#endif
}
