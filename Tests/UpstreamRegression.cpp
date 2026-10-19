#include "OpenRHI.h"

#if OPENRHI_ENABLE_AGILITY_SDK_SUPPORT
#    include "OpenRHIAgilitySDK.h"
#endif

#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIHelper.h"
#include "Extensions/OpenRHIRayTracing.h"
#include "Extensions/OpenRHIStreamer.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

using namespace openrhi;

#define REQUIRE(expression) \
    do { \
        if (!(expression)) \
            throw std::runtime_error(#expression); \
    } while (false)

static uint64_t AlignSize(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

struct Fixture {
    Device* device = nullptr;
    CoreInterface core = {};
    StreamerInterface streaming = {};
    Queue* queue = nullptr;
    CommandAllocator* allocator = nullptr;
    CommandBuffer* command = nullptr;
    Fence* fence = nullptr;
    Streamer* streamer = nullptr;
    std::vector<Texture*> textures;
    std::vector<Buffer*> buffers;
    uint64_t fenceValue = 0;
    uint32_t unexpectedErrors = 0;
    uint32_t expectedErrors = 0;
    uint32_t vulkanMinorVersion = 0;
    bool expectingError = false;
    char lastError[512] = {};

    static void OPENRHI_CALL MessageCallback(Message type, const char*, uint32_t, const char* message, void* context) {
        auto& fixture = *(Fixture*)context;

        if (type == Message::INFO) {
            const char* version = std::strstr(message, "Using Vulkan v1.");

            if (version)
                std::sscanf(version, "Using Vulkan v1.%u", &fixture.vulkanMinorVersion);
        }

        if (type != Message::ERROR)
            return;

        std::snprintf(fixture.lastError, sizeof(fixture.lastError), "%s", message);

        if (fixture.expectingError)
            fixture.expectedErrors++;
        else {
            fixture.unexpectedErrors++;
            std::fprintf(stderr, "%s\n", message);
        }
    }

    static void OPENRHI_CALL AbortExecution(void*) {
    }

    ~Fixture() {
        if (!device)
            return;

        if (core.DeviceWaitIdle)
            core.DeviceWaitIdle(device);

        if (streamer)
            streaming.DestroyStreamer(streamer);

        for (auto texture : textures)
            core.DestroyTexture(texture);

        for (auto buffer : buffers)
            core.DestroyBuffer(buffer);

        if (fence)
            core.DestroyFence(fence);

        if (command)
            core.DestroyCommandBuffer(command);

        if (allocator)
            core.DestroyCommandAllocator(allocator);

        openrhiDestroyDevice(device);
    }

    void Initialize(GraphicsAPI api, const AdapterDesc& adapter) {
        DeviceCreationDesc creation = {};
        creation.graphicsAPI = api;
        creation.adapterDesc = &adapter;
        creation.enableOpenRHIValidation = true;
        creation.enableGraphicsAPIValidation = true;
        creation.callbackInterface = {MessageCallback, AbortExecution, this};
        REQUIRE(openrhiCreateDevice(creation, device) == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(CoreInterface), &core) == Result::SUCCESS);
        REQUIRE(core.GetQueue(*device, QueueType::GRAPHICS, 0, queue) == Result::SUCCESS);
        REQUIRE(core.CreateCommandAllocator(*queue, allocator) == Result::SUCCESS);
        REQUIRE(core.CreateCommandBuffer(*allocator, command) == Result::SUCCESS);
        REQUIRE(core.CreateFence(*device, 0, fence) == Result::SUCCESS);
    }

    void Begin() {
        core.ResetCommandAllocator(*allocator);
        REQUIRE(core.BeginCommandBuffer(*command, nullptr) == Result::SUCCESS);
    }

    void Submit() {
        REQUIRE(core.EndCommandBuffer(*command) == Result::SUCCESS);
        FenceSubmitDesc signal = {fence, ++fenceValue, StageBits::ALL};
        QueueSubmitDesc submit = {};
        submit.commandBuffers = &command;
        submit.commandBufferNum = 1;
        submit.signalFences = &signal;
        submit.signalFenceNum = 1;
        REQUIRE(core.QueueSubmit(*queue, submit) == Result::SUCCESS);
        core.Wait(*fence, fenceValue);
    }

    void TextureViewLayouts() {
        TextureDesc desc = {};
        desc.type = TextureType::TEXTURE_2D;
        desc.format = GetSupportedDepthFormat(core, *device, 24, true);
        desc.usage = TextureUsageBits::DEPTH_STENCIL_ATTACHMENT | TextureUsageBits::SHADER_RESOURCE;
        desc.width = desc.height = 16;
        desc.depth = desc.layerNum = desc.mipNum = desc.sampleNum = 1;
        Texture* texture = nullptr;
        REQUIRE(core.CreateCommittedTexture(*device, MemoryLocation::DEVICE, 0.0f, desc, texture) == Result::SUCCESS);
        textures.push_back(texture);

        TextureViewDesc view = {};
        view.texture = texture;
        view.format = desc.format;
        view.type = TextureView::DEPTH_STENCIL_ATTACHMENT;

        for (Layout layout : {Layout::UNDEFINED, Layout::GENERAL, Layout::DEPTH_STENCIL_ATTACHMENT,
                 Layout::DEPTH_STENCIL_READONLY, Layout::DEPTH_READONLY_STENCIL_ATTACHMENT, Layout::DEPTH_ATTACHMENT_STENCIL_READONLY}) {
            view.layout = layout;
            Descriptor* descriptor = nullptr;
            REQUIRE(core.CreateTextureView(view, descriptor) == Result::SUCCESS);
            core.DestroyDescriptor(descriptor);
        }

        view.type = TextureView::TEXTURE;

        for (PlaneBits planes : {PlaneBits::DEPTH, PlaneBits::STENCIL}) {
            view.planes = planes;

            for (Layout layout : {Layout::GENERAL, Layout::SHADER_RESOURCE, Layout::DEPTH_STENCIL_READONLY,
                     planes == PlaneBits::DEPTH ? Layout::DEPTH_READONLY_STENCIL_ATTACHMENT : Layout::DEPTH_ATTACHMENT_STENCIL_READONLY}) {
                view.layout = layout;
                Descriptor* descriptor = nullptr;
                REQUIRE(core.CreateTextureView(view, descriptor) == Result::SUCCESS);
                core.DestroyDescriptor(descriptor);
            }
        }

        for (Layout layout : {Layout::COLOR_ATTACHMENT, Layout::COPY_SOURCE, Layout::PRESENT, Layout::MAX_NUM,
                 Layout::DEPTH_READONLY_STENCIL_ATTACHMENT}) {
            view.layout = layout;
            Descriptor* descriptor = nullptr;
            const uint32_t before = expectedErrors;
            expectingError = true;
            const Result result = core.CreateTextureView(view, descriptor);
            expectingError = false;
            REQUIRE(result == Result::INVALID_ARGUMENT);
            REQUIRE(!descriptor && expectedErrors == before + 1);
            REQUIRE(std::strstr(lastError, "layout"));
        }

        std::puts("Explicit depth/stencil view layouts and validation: PASS");
    }

    void GeometrySentinels() {
        if (!core.GetDeviceDesc(*device).tiers.rayTracing) {
            std::puts("Geometry sentinels: skipped (ray tracing unavailable)");

            return;
        }

        RayTracingInterface ray = {};
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(RayTracingInterface), &ray) == Result::SUCCESS);
        BottomLevelGeometryDesc geometry = {};
        geometry.type = BottomLevelGeometryType::TRIANGLES;
        geometry.triangles.vertexBuffer = geometry.triangles.indexBuffer = geometry.triangles.transformBuffer = HAS_BUFFER;
        geometry.triangles.vertexNum = geometry.triangles.indexNum = 3;
        geometry.triangles.vertexFormat = Format::RGB32_SFLOAT;
        geometry.triangles.vertexStride = 12;
        geometry.triangles.indexType = IndexType::UINT32;
        AccelerationStructureDesc desc = {};
        desc.type = AccelerationStructureType::BOTTOM_LEVEL;
        desc.geometries = &geometry;
        desc.geometryOrInstanceNum = 1;
        AccelerationStructure* accelerationStructure = nullptr;
        REQUIRE(ray.CreateAccelerationStructure(*device, desc, accelerationStructure) == Result::SUCCESS);
        ray.DestroyAccelerationStructure(accelerationStructure);

        geometry = {};
        geometry.type = BottomLevelGeometryType::AABBS;
        geometry.aabbs.buffer = HAS_BUFFER;
        geometry.aabbs.num = 1;
        geometry.aabbs.stride = sizeof(BottomLevelAabb);
        REQUIRE(ray.CreateAccelerationStructure(*device, desc, accelerationStructure) == Result::SUCCESS);
        ray.DestroyAccelerationStructure(accelerationStructure);

        Begin();
        BuildBottomLevelAccelerationStructureDesc build = {};
        build.geometryNum = 1;
        build.geometries = &geometry;
        BottomLevelTrianglesMicromapDesc micromap = {};

        for (uint32_t slot = 0; slot < 5; slot++) {
            geometry = {};
            geometry.type = slot == 3 ? BottomLevelGeometryType::AABBS : BottomLevelGeometryType::TRIANGLES;

            if (slot == 0)
                geometry.triangles.vertexBuffer = HAS_BUFFER;
            else if (slot == 1)
                geometry.triangles.indexBuffer = HAS_BUFFER;
            else if (slot == 2)
                geometry.triangles.transformBuffer = HAS_BUFFER;
            else if (slot == 3)
                geometry.aabbs.buffer = HAS_BUFFER;
            else {
                micromap.indexBuffer = HAS_BUFFER;
                geometry.triangles.micromap = &micromap;
            }

            const uint32_t before = expectedErrors;
            expectingError = true;
            ray.CmdBuildBottomLevelAccelerationStructures(*command, &build, 1);
            expectingError = false;
            REQUIRE(expectedErrors == before + 1 && std::strstr(lastError, "HAS_BUFFER"));
        }

        Submit();
        std::puts("Geometry creation preserves sentinels; recorded builds reject every sentinel slot: PASS");
    }

    void StreamCompressedTexture(Format format, Dim_t mip) {
        const auto& deviceDesc = core.GetDeviceDesc(*device);
        const auto& props = *openrhiGetFormatProps(format);
        const Dim_t width = Dim_t(8 >> mip);
        const Dim_t height = Dim_t(8 >> mip);
        const uint32_t rowSize = ((width + props.blockWidth - 1) / props.blockWidth) * props.stride;
        const uint32_t rowNum = (height + props.blockHeight - 1) / props.blockHeight;
        const uint32_t sourcePitch = rowSize + 16;
        std::vector<uint8_t> source(std::max(size_t(sourcePitch * height + width * props.stride), size_t(256)), 0xCD);

        for (uint32_t row = 0; row < rowNum; row++) {
            for (uint32_t x = 0; x < rowSize; x++)
                source[row * sourcePitch + x] = uint8_t(row * 37 + x);
        }

        TextureDesc desc = {};
        desc.type = TextureType::TEXTURE_2D;
        desc.format = format;
        desc.usage = TextureUsageBits::SHADER_RESOURCE;
        desc.width = desc.height = 8;
        desc.depth = desc.layerNum = desc.sampleNum = 1;
        desc.mipNum = 3;
        Texture* texture = nullptr;
        REQUIRE(core.CreateCommittedTexture(*device, MemoryLocation::DEVICE, 0.0f, desc, texture) == Result::SUCCESS);
        textures.push_back(texture);

        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(StreamerInterface), &streaming) == Result::SUCCESS);
        StreamerDesc streamerDesc = {};
        streamerDesc.constantBufferMemoryLocation = MemoryLocation::HOST_UPLOAD;
        streamerDesc.dynamicBufferMemoryLocation = MemoryLocation::HOST_UPLOAD;
        streamerDesc.queuedFrameNum = 2;
        REQUIRE(streaming.CreateStreamer(*device, streamerDesc, streamer) == Result::SUCCESS);

        StreamTextureDataDesc upload = {};
        upload.data = source.data();
        upload.dataRowPitch = sourcePitch;
        upload.dataSlicePitch = sourcePitch * rowNum;
        upload.copyBatch = streaming.BeginStreamerCopyBatch(*streamer);
        upload.dstTexture = texture;
        upload.dstRegion.mipOffset = mip;
        const BufferOffset streamed = streaming.StreamTextureData(*streamer, upload);
        REQUIRE(streamed.buffer);

        const uint32_t rowPitch = uint32_t(AlignSize(rowSize, deviceDesc.memoryAlignment.uploadBufferTextureRow));
        const uint32_t slicePitch = uint32_t(AlignSize(uint64_t(rowPitch) * rowNum, deviceDesc.memoryAlignment.uploadBufferTextureSlice));
        const uint8_t marker = 0xA5;
        DataSize chunk = {&marker, 1};
        StreamBufferDataDesc markerDesc = {};
        markerDesc.dataChunks = &chunk;
        markerDesc.dataChunkNum = markerDesc.placementAlignment = 1;
        const BufferOffset next = streaming.StreamBufferData(*streamer, markerDesc);
        REQUIRE(next.buffer == streamed.buffer);
        REQUIRE(next.offset == streamed.offset + slicePitch);

        BufferDesc readbackDesc = {};
        readbackDesc.size = slicePitch;
        Buffer* readback = nullptr;
        REQUIRE(core.CreateCommittedBuffer(*device, MemoryLocation::HOST_READBACK, 0.0f, readbackDesc, readback) == Result::SUCCESS);
        buffers.push_back(readback);

        Begin();
        TextureBarrierDesc barrier = {};
        barrier.texture = texture;
        barrier.after = {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::COPY};
        barrier.mipOffset = mip;
        barrier.mipNum = barrier.layerNum = 1;
        BarrierDesc batch = {};
        batch.textures = &barrier;
        batch.textureNum = 1;
        core.CmdBarrier(*command, batch);
        streaming.CmdCopyStreamedData(*command, *streamer, upload.copyBatch);
        barrier.before = barrier.after;
        barrier.after = {AccessBits::COPY_SOURCE, Layout::COPY_SOURCE, StageBits::COPY};
        core.CmdBarrier(*command, batch);
        TextureDataLayoutDesc readbackLayout = {0, rowPitch, slicePitch};
        core.CmdReadbackTextureToBuffer(*command, *readback, readbackLayout, *texture, upload.dstRegion);
        Submit();

        const uint8_t* mapped = (const uint8_t*)core.MapBuffer(*readback, 0, slicePitch);
        REQUIRE(mapped);

        for (uint32_t row = 0; row < rowNum; row++)
            REQUIRE(std::memcmp(mapped + row * rowPitch, source.data() + row * sourcePitch, rowSize) == 0);

        core.UnmapBuffer(*readback);
        streaming.EndStreamerFrame(*streamer);
        streaming.DestroyStreamer(streamer);
        streamer = nullptr;
        std::printf("Compressed streamer %s mip %u: PASS\n", props.name, mip);
    }
};

int main(int argc, char** argv) {
    GraphicsAPI api = GraphicsAPI::D3D12;

    if (argc > 1 && std::strcmp(argv[1], "VK") == 0)
        api = GraphicsAPI::VK;
    else if (argc > 1 && std::strcmp(argv[1], "METAL") == 0)
        api = GraphicsAPI::METAL;

    try {
        uint32_t adapterNum = 0;
        REQUIRE(openrhiEnumerateAdapters(nullptr, adapterNum) == Result::SUCCESS);
        std::vector<AdapterDesc> adapters(adapterNum);
        REQUIRE(openrhiEnumerateAdapters(adapters.data(), adapterNum) == Result::SUCCESS);

        const auto selected = std::find_if(adapters.begin(), adapters.end(), [api](const AdapterDesc& adapter) {
            return (uint8_t(adapter.supportedGraphicsAPIs) & uint8_t(api)) != 0;
        });

        if (selected == adapters.end()) {
            std::puts("SKIP: no compatible adapter");

            return 77;
        }

        Fixture fixture;
        fixture.Initialize(api, *selected);
        const auto& identity = fixture.core.GetDeviceDesc(*fixture.device).adapterDesc;
        std::printf("Adapter: %s; vendor=0x%X device=0x%X driver=%llu %s\n", identity.name, identity.vendorId, identity.deviceId,
            (unsigned long long)identity.nativeDriverVersion, identity.driverDescription);

        if (api == GraphicsAPI::VK && argc > 2 && std::strcmp(argv[2], "--instance-cap") == 0) {
            REQUIRE(fixture.vulkanMinorVersion == 3);
            std::puts("Effective Vulkan version respects the 1.3 instance cap: PASS");
        }
        fixture.TextureViewLayouts();
        fixture.GeometrySentinels();

        if (fixture.core.GetDeviceDesc(*fixture.device).features.textureCompressionBC) {
            fixture.StreamCompressedTexture(Format::BC1_RGBA_UNORM, 0);
            fixture.StreamCompressedTexture(Format::BC3_RGBA_UNORM, 2);
        }

        if (fixture.core.GetDeviceDesc(*fixture.device).features.textureCompressionETC2)
            fixture.StreamCompressedTexture(Format::ETC2_RGB8_UNORM, 0);

        if (fixture.core.GetDeviceDesc(*fixture.device).features.textureCompressionASTC) {
            fixture.StreamCompressedTexture(Format::ASTC_5X4_UNORM, 0);
            fixture.StreamCompressedTexture(Format::ASTC_5X4_UNORM, 2);
        }

        REQUIRE(fixture.unexpectedErrors == 0);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Upstream regression: %s\n", error.what());

        return 1;
    }

    return 0;
}
