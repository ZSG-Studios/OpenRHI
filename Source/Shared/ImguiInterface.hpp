// © 2025-2026 NVIDIA Corporation

#if OPENRHI_ENABLE_IMGUI_EXTENSION
#    include "Imgui_fs.h"
#    include "Imgui_vs.h"

constexpr uint32_t IMGUI_SAMPLER_SET = 0;
constexpr uint32_t IMGUI_TEXTURE_SET = 1;

struct ImguiConstants {
    float invDisplayWidth;
    float invDisplayHeight;
    float hdrScale;
    float gamma;
};

// Copied from Imgui // TODO: always keep in sync with latest

typedef uint16_t ImDrawIdx;
typedef uint64_t ImTextureID;

constexpr ImTextureID ImTextureID_Invalid = 0;

enum ImTextureFormat {
    ImTextureFormat_RGBA32,
    ImTextureFormat_Alpha8,
};

enum ImTextureStatus {
    ImTextureStatus_OK,
    ImTextureStatus_Destroyed,
    ImTextureStatus_WantCreate,
    ImTextureStatus_WantUpdates,
    ImTextureStatus_WantDestroy,
};

template <typename T>
struct ImVector {
    int32_t Size;
    int32_t Capacity;
    T* Data;
};

struct ImVec4 {
    float x, y, z, w;
};

struct ImVec2 {
    float x, y;
};

struct ImDrawVert {
    ImVec2 pos;
    ImVec2 uv;
    uint32_t col;
};

struct ImTextureRect {
    uint16_t x, y, w, h;
};

struct ImTextureData {
    int UniqueID;
    ImTextureStatus Status;
    void* BackendUserData;
    void* QueueUserData;
    ImTextureID TexID;
    ImTextureFormat Format;
    int Width;
    int Height;
    int BytesPerPixel;
    unsigned char* Pixels;
    ImTextureRect UsedRect;
    ImTextureRect UpdateRect;
    ImVector<ImTextureRect> Updates;
    int UnusedFrames;
    unsigned short RefCount;
    bool UseColors;
    bool WantDestroyNextFrame;
};

struct ImTextureRef {
    ImTextureData* TexData;
    ImTextureID TexID;
};

struct ImDrawCmd {
    ImVec4 ClipRect;
    ImTextureRef TexRef;
    uint32_t VtxOffset;
    uint32_t IdxOffset;
    uint32_t ElemCount;
    void* UserCallback;
    void* UserCallbackData;
    int32_t UserCallbackDataSize;
    int32_t UserCallbackDataOffset;
};

struct ImDrawList {
    ImVector<ImDrawCmd> CmdBuffer;
    ImVector<ImDrawIdx> IdxBuffer;
    ImVector<ImDrawVert> VtxBuffer;

    // rest is unreferenced
};

struct ImguiDrawCmd {
    ImVec4 clipRect;
    DrawIndexedDesc drawIndexedDesc;
    Descriptor* descriptor;
    float hdrScale;
    bool isHdrScaleOverride;
};

static inline uint64_t GetImguiTextureKey(const ImTextureData& imTextureData) {
    return (uint64_t)&imTextureData | ((uint64_t)imTextureData.UniqueID << 54ull);
}

// Implementation

Lock ImguiImpl::s_TextureLock;

ImguiImpl::~ImguiImpl() {
    for (auto& entry : m_Textures) {
        m_iCore.DestroyDescriptor(entry.second.descriptor);
        m_iCore.DestroyTexture(entry.second.texture);

        // TODO: need to update "ImTextureData"? saving a pointer to do this is invalid since its memory state is unknown
    }

    for (const ImguiPipeline& entry : m_Pipelines)
        m_iCore.DestroyPipeline(entry.pipeline);

    m_iCore.DestroyPipelineLayout(m_PipelineLayout);
    m_iCore.DestroyDescriptorPool(m_DescriptorPool);
    m_iCore.DestroyDescriptor(m_Sampler);
}

Result ImguiImpl::Create(const ImguiDesc& imguiDesc) {
    { // Get streamer interface
        Result result = openrhiGetInterface(m_Device, OPENRHI_INTERFACE(StreamerInterface), &m_iStreamer);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Create sampller
        SamplerDesc viewDesc = {};
        viewDesc.filters.min = Filter::LINEAR;
        viewDesc.filters.mag = Filter::LINEAR;
        viewDesc.addressModes.u = AddressMode::REPEAT;
        viewDesc.addressModes.v = AddressMode::REPEAT;

        Result result = m_iCore.CreateSampler(m_Device, viewDesc, m_Sampler);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Pipeline layout
        RootConstantDesc rootConstants = {};
        rootConstants.registerIndex = 0;
        rootConstants.shaderStages = StageBits::VERTEX_SHADER;
        rootConstants.size = sizeof(ImguiConstants);

        const DescriptorRangeDesc descriptorSet0Ranges[] = {
            {1, 1, DescriptorType::SAMPLER, StageBits::FRAGMENT_SHADER},
        };

        const DescriptorRangeDesc descriptorSet1Ranges[] = {
            {0, 1, DescriptorType::TEXTURE, StageBits::FRAGMENT_SHADER, DescriptorRangeBits::ALLOW_UPDATE_AFTER_SET},
        };

        DescriptorSetDesc descriptorSetDescs[2] = {};
        {
            descriptorSetDescs[IMGUI_SAMPLER_SET].registerSpace = IMGUI_SAMPLER_SET;
            descriptorSetDescs[IMGUI_SAMPLER_SET].ranges = descriptorSet0Ranges;
            descriptorSetDescs[IMGUI_SAMPLER_SET].rangeNum = GetCountOf(descriptorSet0Ranges);

            descriptorSetDescs[IMGUI_TEXTURE_SET].registerSpace = IMGUI_TEXTURE_SET;
            descriptorSetDescs[IMGUI_TEXTURE_SET].ranges = descriptorSet1Ranges;
            descriptorSetDescs[IMGUI_TEXTURE_SET].rangeNum = GetCountOf(descriptorSet1Ranges);
            descriptorSetDescs[IMGUI_TEXTURE_SET].flags = DescriptorSetBits::ALLOW_UPDATE_AFTER_SET;
        }

        PipelineLayoutDesc pipelineLayoutDesc = {};
        pipelineLayoutDesc.rootRegisterSpace = 0;
        pipelineLayoutDesc.rootConstants = &rootConstants;
        pipelineLayoutDesc.rootConstantNum = 1;
        pipelineLayoutDesc.descriptorSets = descriptorSetDescs;
        pipelineLayoutDesc.descriptorSetNum = GetCountOf(descriptorSetDescs);
        pipelineLayoutDesc.shaderStages = StageBits::VERTEX_SHADER | StageBits::FRAGMENT_SHADER;
        pipelineLayoutDesc.flags = PipelineLayoutBits::IGNORE_GLOBAL_SPIRV_OFFSETS;

        Result result = m_iCore.CreatePipelineLayout(m_Device, pipelineLayoutDesc, m_PipelineLayout);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Descriptor pool
        DescriptorPoolDesc descriptorPoolDesc = {};

        // Static
        descriptorPoolDesc.descriptorSetMaxNum = 1;
        descriptorPoolDesc.samplerMaxNum = 1;

        // Dynamic
        uint32_t dynamicPoolSize = imguiDesc.descriptorPoolSize ? imguiDesc.descriptorPoolSize : 128;
        m_DescriptorSets1.resize(dynamicPoolSize);

        descriptorPoolDesc.descriptorSetMaxNum += dynamicPoolSize;
        descriptorPoolDesc.textureMaxNum += dynamicPoolSize;
        descriptorPoolDesc.flags = DescriptorPoolBits::ALLOW_UPDATE_AFTER_SET;

        Result result = m_iCore.CreateDescriptorPool(m_Device, descriptorPoolDesc, m_DescriptorPool);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Descriptor sets
        Result result = m_iCore.AllocateDescriptorSets(*m_DescriptorPool, *m_PipelineLayout, IMGUI_SAMPLER_SET, &m_DescriptorSet0_sampler, 1, 0);
        if (result != Result::SUCCESS)
            return result;

        result = m_iCore.AllocateDescriptorSets(*m_DescriptorPool, *m_PipelineLayout, IMGUI_TEXTURE_SET, m_DescriptorSets1.data(), (uint32_t)m_DescriptorSets1.size(), 0);
        if (result != Result::SUCCESS)
            return result;
    }

    { // Update static set with sampler
        UpdateDescriptorRangeDesc updateDescriptorRangeDesc = {m_DescriptorSet0_sampler, 0, 0, &m_Sampler, 1};
        m_iCore.UpdateDescriptorRanges(&updateDescriptorRangeDesc, 1);
    }

    return Result::SUCCESS;
}

void ImguiImpl::CmdCopyData(CommandBuffer& commandBuffer, Streamer& streamer, const CopyImguiDataDesc& copyImguiDataDesc, ImguiRenderData& imguiRenderData) {
    ExclusiveScope lock(m_Lock);

    CmdCopyDataInternal(commandBuffer, streamer, copyImguiDataDesc, imguiRenderData);
}

void ImguiImpl::CmdDraw(CommandBuffer& commandBuffer, const ImguiRenderData& imguiRenderData, const DrawImguiDesc& drawImguiDesc) {
    ExclusiveScope lock(m_Lock);

    CmdDrawInternal(commandBuffer, imguiRenderData, drawImguiDesc);
}

void ImguiImpl::CmdCopyDataInternal(CommandBuffer& commandBuffer, Streamer& streamer, const CopyImguiDataDesc& copyImguiDataDesc, ImguiRenderData& imguiRenderData) {
    imguiRenderData = {};
    imguiRenderData.imgui = (Imgui*)this;

    if (!copyImguiDataDesc.drawListNum)
        return;

    StreamerCopyBatch copyBatch = 0;

    Scratch<TextureBarrierDesc> textureBarriers = OPENRHI_ALLOCATE_SCRATCH((DeviceBase&)m_Device, TextureBarrierDesc, copyImguiDataDesc.textureNum);
    uint32_t textureBarrierNum = 0;

    const AccessLayoutStage copyState = {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::COPY};
    const AccessLayoutStage drawState = {AccessBits::SHADER_RESOURCE, Layout::SHADER_RESOURCE, StageBits::FRAGMENT_SHADER};

    // Update textures
    for (uint32_t i = 0; i < copyImguiDataDesc.textureNum; i++) {
        ImTextureData* imTextureData = copyImguiDataDesc.textures[i];
        uint64_t key = GetImguiTextureKey(*imTextureData);

        ImTextureStatus currentStatus;
        uint64_t updateTick;

        { // Phase 1: satisfy ImGui
            ExclusiveScope lock(s_TextureLock);

            currentStatus = imTextureData->Status;

            // Destroy
            if (currentStatus == ImTextureStatus_WantDestroy && imTextureData->UnusedFrames > 8) { // TODO: keep an eye on 8...
                imTextureData->TexID = ImTextureID_Invalid;
                imTextureData->BackendUserData = nullptr; // reset counter
                imTextureData->Status = ImTextureStatus_Destroyed;
            }

            // Create or update
            if (currentStatus == ImTextureStatus_WantCreate || currentStatus == ImTextureStatus_WantUpdates) {
                imTextureData->TexID = key;
                imTextureData->BackendUserData = (void*)((uint64_t)imTextureData->BackendUserData + 1); // increment counter
                imTextureData->Status = ImTextureStatus_OK;
            }

            updateTick = (uint64_t)imTextureData->BackendUserData;
        }

        { // Phase 2: real logic - OpenRHI supports rendering of the same UI instance on multiple devices
            auto entry = m_Textures.find(key);
            if (entry == m_Textures.end())
                entry = m_Textures.insert({key, {}}).first;

            ImguiTexture& imguiTexture = entry->second;
            Format format = imTextureData->Format == ImTextureFormat_RGBA32 ? Format::RGBA8_UNORM : Format::R8_UNORM;

            // Destroy
            if (updateTick == 0) {
                OPENRHI_CHECK(imTextureData->Status == ImTextureStatus_Destroyed, "Unexpected");

                // TODO: this is valid only in the current frame. Otherwise an "ImguiImpl" instance can get a zombie texture in "m_Textures" because
                // starting from the next frame "copyImguiDataDesc.textures" won't contain textures with "ImTextureStatus_Destroyed" status
                m_iCore.DestroyDescriptor(imguiTexture.descriptor);
                m_iCore.DestroyTexture(imguiTexture.texture);

                m_Textures.erase(key);

                continue;
            }

            // A newly added texture requires creation
            if (imguiTexture.updateTick == 0) {
                // Create texture
                TextureDesc textureDesc = {};
                textureDesc.type = TextureType::TEXTURE_2D;
                textureDesc.usage = TextureUsageBits::SHADER_RESOURCE;
                textureDesc.format = format;
                textureDesc.width = (Dim_t)imTextureData->Width;
                textureDesc.height = (Dim_t)imTextureData->Height;

                Result result = m_iCore.CreateCommittedTexture(m_Device, MemoryLocation::DEVICE, 0.0f, textureDesc, imguiTexture.texture);
                OPENRHI_CHECK(result == Result::SUCCESS, "Unexpected");

                // Create descriptor
                TextureViewDesc viewDesc = {};
                viewDesc.texture = imguiTexture.texture;
                viewDesc.type = TextureView::TEXTURE;
                viewDesc.format = format;

                result = m_iCore.CreateTextureView(viewDesc, imguiTexture.descriptor);
                OPENRHI_CHECK(result == Result::SUCCESS, "Unexpected");
            }

            // Update if needed
            OPENRHI_CHECK(imguiTexture.texture && imguiTexture.descriptor, "Unexpected");

            if (imguiTexture.updateTick < updateTick) {
                if (!copyBatch)
                    copyBatch = m_iStreamer.BeginStreamerCopyBatch(streamer);

                const FormatProps& formatProps = GetFormatProps(format);

                if (imguiTexture.updateTick == 0) {
                    // A newly created texture must be fully updated
                    StreamTextureDataDesc streamTextureDataDesc = {};
                    streamTextureDataDesc.data = imTextureData->Pixels;
                    streamTextureDataDesc.dataRowPitch = imTextureData->Width * formatProps.stride;
                    streamTextureDataDesc.dataSlicePitch = imTextureData->Height * streamTextureDataDesc.dataRowPitch;
                    streamTextureDataDesc.copyBatch = copyBatch;
                    streamTextureDataDesc.dstTexture = imguiTexture.texture;

                    m_iStreamer.StreamTextureData(streamer, streamTextureDataDesc);
                } else if (currentStatus == ImTextureStatus_WantUpdates) {
                    // "Updates" vector is valid only "here and now", i.e. first renderer encountering "ImTextureStatus_WantUpdates" takes it // TODO: improve
                    for (int32_t j = 0; j < imTextureData->Updates.Size; j++) {
                        const ImTextureRect& rect = imTextureData->Updates.Data[j];

                        StreamTextureDataDesc streamTextureDataDesc = {};
                        streamTextureDataDesc.data = imTextureData->Pixels + (rect.x + rect.y * imTextureData->Width) * formatProps.stride;
                        streamTextureDataDesc.dataRowPitch = imTextureData->Width * formatProps.stride;
                        streamTextureDataDesc.dataSlicePitch = imTextureData->Height * streamTextureDataDesc.dataRowPitch;
                        streamTextureDataDesc.copyBatch = copyBatch;
                        streamTextureDataDesc.dstTexture = imguiTexture.texture;
                        streamTextureDataDesc.dstRegion.x = rect.x;
                        streamTextureDataDesc.dstRegion.y = rect.y;
                        streamTextureDataDesc.dstRegion.width = rect.w;
                        streamTextureDataDesc.dstRegion.height = rect.h;

                        m_iStreamer.StreamTextureData(streamer, streamTextureDataDesc);
                    }
                } else {
                    // If a texture is outdated, use "UsedRect" since "Updates" are valid only in the current frame
                    const ImTextureRect& rect = imTextureData->UsedRect;

                    StreamTextureDataDesc streamTextureDataDesc = {};
                    streamTextureDataDesc.data = imTextureData->Pixels + (rect.x + rect.y * imTextureData->Width) * formatProps.stride;
                    streamTextureDataDesc.dataRowPitch = imTextureData->Width * formatProps.stride;
                    streamTextureDataDesc.dataSlicePitch = imTextureData->Height * streamTextureDataDesc.dataRowPitch;
                    streamTextureDataDesc.copyBatch = copyBatch;
                    streamTextureDataDesc.dstTexture = imguiTexture.texture;
                    streamTextureDataDesc.dstRegion.x = rect.x;
                    streamTextureDataDesc.dstRegion.y = rect.y;
                    streamTextureDataDesc.dstRegion.width = rect.w;
                    streamTextureDataDesc.dstRegion.height = rect.h;

                    m_iStreamer.StreamTextureData(streamer, streamTextureDataDesc);
                }

                { // Add a barrier
                    TextureBarrierDesc& textureBarrier = textureBarriers[textureBarrierNum++];

                    textureBarrier = {};
                    textureBarrier.texture = imguiTexture.texture;
                    textureBarrier.before = drawState;
                    textureBarrier.after = copyState;

                    if (imguiTexture.updateTick == 0)
                        textureBarrier.before = {};
                }

                // Mark as "up to date"
                imguiTexture.updateTick = updateTick;
            }
        }
    }

    { // Stream draw commands and buffer data
        uint32_t drawCmdNum = 0;
        for (uint32_t n = 0; n < copyImguiDataDesc.drawListNum; n++)
            drawCmdNum += copyImguiDataDesc.drawLists[n]->CmdBuffer.Size;

        Scratch<ImguiDrawCmd> drawCmds = OPENRHI_ALLOCATE_SCRATCH((DeviceBase&)m_Device, ImguiDrawCmd, drawCmdNum);

        uint32_t drawCmdIndex = 0;
        uint32_t vertexOffset = 0;
        uint32_t indexOffset = 0;
        for (uint32_t n = 0; n < copyImguiDataDesc.drawListNum; n++) {
            const ImDrawList* imDrawList = copyImguiDataDesc.drawLists[n];

            for (int32_t i = 0; i < imDrawList->CmdBuffer.Size; i++) {
                const ImDrawCmd& src = imDrawList->CmdBuffer.Data[i];
                ImguiDrawCmd& dst = drawCmds[drawCmdIndex++];

                dst = {};
                dst.clipRect = src.ClipRect;

                if (src.UserCallback) {
                    dst.hdrScale = *(float*)&src.UserCallbackData;
                    dst.isHdrScaleOverride = true;
                } else {
                    dst.drawIndexedDesc.indexNum = src.ElemCount;
                    dst.drawIndexedDesc.instanceNum = 1;
                    dst.drawIndexedDesc.baseIndex = src.IdxOffset + indexOffset;
                    dst.drawIndexedDesc.baseVertex = src.VtxOffset + vertexOffset;

                    if (src.TexRef.TexData) {
                        auto entry = m_Textures.find(GetImguiTextureKey(*src.TexRef.TexData));
                        OPENRHI_CHECK(entry != m_Textures.end(), "Unexpected");
                        dst.descriptor = entry->second.descriptor;
                    } else
                        dst.descriptor = (Descriptor*)src.TexRef.TexID;
                }
            }

            vertexOffset += imDrawList->VtxBuffer.Size;
            indexOffset += imDrawList->IdxBuffer.Size;
        }

        const void* drawCommands = nullptr;
        if (drawCmdNum)
            drawCommands = m_iStreamer.StreamHostData(streamer, drawCmds, drawCmdNum * sizeof(ImguiDrawCmd), alignof(ImguiDrawCmd));

        if (drawCommands) {
            uint32_t dataChunkNum = copyImguiDataDesc.drawListNum * 2;
            Scratch<DataSize> dataChunks = OPENRHI_ALLOCATE_SCRATCH((DeviceBase&)m_Device, DataSize, dataChunkNum);

            StreamBufferDataDesc streamBufferDataDesc = {};
            streamBufferDataDesc.dataChunkNum = dataChunkNum;
            streamBufferDataDesc.dataChunks = dataChunks;
            streamBufferDataDesc.placementAlignment = std::max((uint32_t)alignof(ImDrawVert), (uint32_t)alignof(ImDrawIdx));

            uint64_t totalVertexDataSize = 0;
            for (uint32_t n = 0; n < copyImguiDataDesc.drawListNum; n++) {
                const ImDrawList* imDrawList = copyImguiDataDesc.drawLists[n];

                DataSize& vertexDataChunk = dataChunks[n];
                vertexDataChunk = {};
                vertexDataChunk.data = imDrawList->VtxBuffer.Data;
                vertexDataChunk.size = imDrawList->VtxBuffer.Size * sizeof(ImDrawVert);

                DataSize& indexDataChunk = dataChunks[copyImguiDataDesc.drawListNum + n];
                indexDataChunk = {};
                indexDataChunk.data = imDrawList->IdxBuffer.Data;
                indexDataChunk.size = imDrawList->IdxBuffer.Size * sizeof(ImDrawIdx);

                totalVertexDataSize += vertexDataChunk.size;
            }

            BufferOffset vertices = m_iStreamer.StreamBufferData(streamer, streamBufferDataDesc);

            if (vertices.buffer) {
                imguiRenderData.drawCommands = drawCommands;
                imguiRenderData.vertices = vertices;
                imguiRenderData.indexBufferOffset = vertices.offset + totalVertexDataSize;
                imguiRenderData.drawCmdNum = drawCmdNum;
            }
        }
    }

    // Copy texture data
    if (textureBarrierNum) {
        BarrierDesc barrierDesc = {};
        barrierDesc.textureNum = textureBarrierNum;
        barrierDesc.textures = textureBarriers;

        m_iCore.CmdBarrier(commandBuffer, barrierDesc);

        m_iStreamer.CmdCopyStreamedData(commandBuffer, streamer, copyBatch);

        for (uint32_t i = 0; i < textureBarrierNum; i++) {
            textureBarriers[i].before = copyState;
            textureBarriers[i].after = drawState;
        }

        m_iCore.CmdBarrier(commandBuffer, barrierDesc);
    }
}

void ImguiImpl::CmdDrawInternal(CommandBuffer& commandBuffer, const ImguiRenderData& imguiRenderData, const DrawImguiDesc& drawImguiDesc) {
    if (!imguiRenderData.drawCmdNum)
        return;

    // Pipeline
    Pipeline* pipeline = nullptr;
    for (size_t i = 0; i < m_Pipelines.size(); i++) {
        const ImguiPipeline& imguiPipeline = m_Pipelines[i];
        if (imguiPipeline.format == drawImguiDesc.attachmentFormat && imguiPipeline.writeAlpha == drawImguiDesc.writeAlpha) {
            pipeline = m_Pipelines[i].pipeline;
            break;
        }
    }

    if (!pipeline) {
        const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);
        MaybeUnused(deviceDesc);

        ShaderDesc shaders[] = {g_Imgui_vs_desc, g_Imgui_fs_desc};

        const VertexAttributeDesc vertexAttributeDesc[] = {
            {{"POSITION", 0}, {0}, offsetof(ImDrawVert, pos), Format::RG32_SFLOAT},
            {{"TEXCOORD", 0}, {1}, offsetof(ImDrawVert, uv), Format::RG32_SFLOAT},
            {{"COLOR", 0}, {2}, offsetof(ImDrawVert, col), Format::RGBA8_UNORM},
        };

        VertexStreamDesc stream = {};
        stream.bindingSlot = 0;
        stream.stride = sizeof(ImDrawVert);

        VertexInputDesc vertexInput = {};
        vertexInput.attributes = vertexAttributeDesc;
        vertexInput.attributeNum = (uint8_t)GetCountOf(vertexAttributeDesc);
        vertexInput.streams = &stream;
        vertexInput.streamNum = 1;

        ColorAttachmentDesc colorAttachment = {};
        colorAttachment.format = drawImguiDesc.attachmentFormat;
        colorAttachment.colorBlend.srcFactor = BlendFactor::SRC_ALPHA,
        colorAttachment.colorBlend.dstFactor = BlendFactor::ONE_MINUS_SRC_ALPHA,
        colorAttachment.colorBlend.op = BlendOp::ADD,
        colorAttachment.alphaBlend.srcFactor = BlendFactor::ONE,
        colorAttachment.alphaBlend.dstFactor = BlendFactor::ONE_MINUS_SRC_ALPHA,
        colorAttachment.alphaBlend.op = BlendOp::ADD,
        colorAttachment.colorWriteMask = drawImguiDesc.writeAlpha ? ColorWriteBits::RGBA : ColorWriteBits::RGB;
        colorAttachment.blendEnabled = true;

        GraphicsPipelineDesc graphicsPipelineDesc = {};
        graphicsPipelineDesc.pipelineLayout = m_PipelineLayout;
        graphicsPipelineDesc.vertexInput = &vertexInput;
        graphicsPipelineDesc.inputAssembly.topology = Topology::TRIANGLE_LIST;
        graphicsPipelineDesc.rasterization.fillMode = FillMode::SOLID;
        graphicsPipelineDesc.rasterization.cullMode = CullMode::NONE;
        graphicsPipelineDesc.outputMerger.colors = &colorAttachment;
        graphicsPipelineDesc.outputMerger.colorNum = 1;
        graphicsPipelineDesc.shaders = shaders;
        graphicsPipelineDesc.shaderNum = GetCountOf(shaders);

        Result result = m_iCore.CreateGraphicsPipeline(m_Device, graphicsPipelineDesc, pipeline);
        OPENRHI_CHECK(result == Result::SUCCESS, "Unexpected");
        MaybeUnused(result);

        m_Pipelines.push_back({pipeline, drawImguiDesc.attachmentFormat, drawImguiDesc.writeAlpha});
    }

    // Setup
    float defaultHdrScale = drawImguiDesc.hdrScale == 0.0f ? 1.0f : drawImguiDesc.hdrScale;

    m_iCore.CmdSetDescriptorPool(commandBuffer, *m_DescriptorPool);
    m_iCore.CmdSetPipelineLayout(commandBuffer, BindPoint::GRAPHICS, *m_PipelineLayout);
    m_iCore.CmdSetPipeline(commandBuffer, *pipeline);
    m_iCore.CmdSetIndexBuffer(commandBuffer, *imguiRenderData.vertices.buffer, imguiRenderData.indexBufferOffset, IndexType::UINT16);

    SetDescriptorSetDesc samplerBindingDesc = {IMGUI_SAMPLER_SET, m_DescriptorSet0_sampler};
    m_iCore.CmdSetDescriptorSet(commandBuffer, samplerBindingDesc);

    VertexBufferDesc vertexBufferDesc = {};
    vertexBufferDesc.buffer = imguiRenderData.vertices.buffer;
    vertexBufferDesc.offset = imguiRenderData.vertices.offset;
    vertexBufferDesc.stride = sizeof(ImDrawVert);

    m_iCore.CmdSetVertexBuffers(commandBuffer, 0, &vertexBufferDesc, 1);

    Viewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = drawImguiDesc.displaySize.w;
    viewport.height = drawImguiDesc.displaySize.h;
    viewport.depthMin = 0.0f;
    viewport.depthMax = 1.0f;

    m_iCore.CmdSetViewports(commandBuffer, &viewport, 1);

    ImguiConstants constants = {};
    constants.hdrScale = defaultHdrScale;
    constants.invDisplayWidth = 1.0f / viewport.width;
    constants.invDisplayHeight = 1.0f / viewport.height;
    constants.gamma = drawImguiDesc.linearColor ? 2.2f : 1.0f;

    SetRootConstantsDesc setRootConstantsDesc = {0, &constants, sizeof(constants)};
    m_iCore.CmdSetRootConstants(commandBuffer, setRootConstantsDesc);

    // For each draw command
    Descriptor* currentDescriptor = nullptr;
    float currentHdrScale = -1.0f;
    float hdrScale = 0.0f;
    const ImguiDrawCmd* drawCmds = (const ImguiDrawCmd*)imguiRenderData.drawCommands;

    for (uint32_t i = 0; i < imguiRenderData.drawCmdNum; i++) {
        const ImguiDrawCmd& drawCmd = drawCmds[i];
        ImVec4 clipRect = drawCmd.clipRect; // min.x, min.y, max.x, max.y

        // VK doesn't allow negative values
        clipRect.x = std::max(clipRect.x, 0.0f);
        clipRect.y = std::max(clipRect.y, 0.0f);

        // Clipped?
        if (clipRect.z <= clipRect.x || clipRect.w <= clipRect.y)
            continue;

        if (drawCmd.isHdrScaleOverride) {
            // Nothing to render, just update HDR scale
            hdrScale = drawCmd.hdrScale;
        } else {
            // Change HDR scale
            if (hdrScale != currentHdrScale) {
                currentHdrScale = hdrScale;

                constants.hdrScale = currentHdrScale == 0.0f ? defaultHdrScale : currentHdrScale;

                m_iCore.CmdSetRootConstants(commandBuffer, setRootConstantsDesc);
            }

            // Change texture
            Descriptor* descriptor = drawCmd.descriptor;

            if (descriptor != currentDescriptor) {
                currentDescriptor = descriptor;

                DescriptorSet* descriptorSet = m_DescriptorSets1[m_DescriptorSetIndex];
                m_DescriptorSetIndex = (m_DescriptorSetIndex + 1) % m_DescriptorSets1.size();

                SetDescriptorSetDesc textureBindingDesc = {IMGUI_TEXTURE_SET, descriptorSet};
                m_iCore.CmdSetDescriptorSet(commandBuffer, textureBindingDesc);

                UpdateDescriptorRangeDesc updateDescriptorRangeDesc = {descriptorSet, 0, 0, &currentDescriptor, 1};
                m_iCore.UpdateDescriptorRanges(&updateDescriptorRangeDesc, 1);
            }

            Rect rect = {
                (int16_t)clipRect.x,
                (int16_t)clipRect.y,
                (Dim_t)(clipRect.z - clipRect.x),
                (Dim_t)(clipRect.w - clipRect.y),
            };

            m_iCore.CmdSetScissors(commandBuffer, &rect, 1);
            m_iCore.CmdDrawIndexed(commandBuffer, drawCmd.drawIndexedDesc);
        }
    }
}

#endif
