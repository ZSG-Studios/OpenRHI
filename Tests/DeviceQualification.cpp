#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#endif

#include "OpenRHI.h"

#if OPENRHI_ENABLE_AGILITY_SDK_SUPPORT
#    include "OpenRHIAgilitySDK.h"
#endif

#include "Extensions/OpenRHIDescriptorHeap.h"
#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIHelper.h"
#include "Extensions/OpenRHIMeshShader.h"
#include "Extensions/OpenRHISwapChain.h"
#include "Extensions/OpenRHIUpscaler.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace openrhi;
using Clock = std::chrono::steady_clock;

static void Require(bool condition, const char* expression) {
    if (!condition)
        throw std::runtime_error(expression);
}

#define REQUIRE(x) Require(bool(x), #x)

static uint32_t validationErrors = 0;

static void OPENRHI_CALL MessageCallback(Message type, const char*, uint32_t, const char* message, void*) {
    if (type == Message::ERROR)
        validationErrors++;
    std::fprintf(stderr, "%s\n", message);
}

static void OPENRHI_CALL NoAbort(void*) {
}

static std::string Quote(const char* value) {
    std::string out = "\"";

    for (; *value; value++) {
        if (*value == '"' || *value == '\\')
            out += '\\';

        if (uint8_t(*value) >= 32)
            out += *value;
    }

    return out + '"';
}

struct Fixture {
    Device* device = nullptr;
    CoreInterface core = {};
    Queue* queue = nullptr;
    CommandAllocator* allocator = nullptr;
    CommandBuffer* command = nullptr;
    Fence* fence = nullptr;
    Buffer* upload = nullptr;
    Buffer* local = nullptr;
    Buffer* readback = nullptr;
    Buffer* timestamps = nullptr;
    QueryPool* queries = nullptr;
    PipelineLayout* layout = nullptr;
    Pipeline* pipeline = nullptr;
    DescriptorPool* pool = nullptr;
    DescriptorHeap* heap = nullptr;
    DescriptorHeapInterface heaps = {};
    DescriptorSet* set = nullptr;
    Descriptor* views[2] = {};
    Buffer* outputs[2] = {};
    Texture* nisTextures[2] = {};
    Descriptor* nisViews[2] = {};
    Buffer* nisPixels = nullptr;
    Upscaler* nis = nullptr;
    UpscalerInterface upscaler = {};
    Texture* color = nullptr;
    Descriptor* colorView = nullptr;
    PipelineLayout* graphicsLayout = nullptr;
    Pipeline* graphicsPipeline = nullptr;
    Buffer* arguments = nullptr;
    Buffer* pixels = nullptr;
    Fence* acquireSemaphore = nullptr;
    Fence* releaseSemaphore = nullptr;
    uint64_t value = 0;

    ~Fixture() {
        if (!device)
            return;

        if (!core.DestroyFence) {
            openrhiDestroyDevice(device);
            return;
        }
        core.DeviceWaitIdle(device);

        if (nis)
            upscaler.DestroyUpscaler(nis);

        for (auto descriptor : nisViews)
            if (descriptor)
                core.DestroyDescriptor(descriptor);

        for (auto texture : nisTextures)
            if (texture)
                core.DestroyTexture(texture);

        if (nisPixels)
            core.DestroyBuffer(nisPixels);

        if (colorView)
            core.DestroyDescriptor(colorView);

        if (graphicsPipeline)
            core.DestroyPipeline(graphicsPipeline);

        if (graphicsLayout)
            core.DestroyPipelineLayout(graphicsLayout);

        if (color)
            core.DestroyTexture(color);

        if (arguments)
            core.DestroyBuffer(arguments);

        if (pixels)
            core.DestroyBuffer(pixels);

        for (auto v : views)
            if (v)
                core.DestroyDescriptor(v);

        if (pipeline)
            core.DestroyPipeline(pipeline);

        if (layout)
            core.DestroyPipelineLayout(layout);

        if (pool)
            core.DestroyDescriptorPool(pool);

        if (heap)
            heaps.DestroyDescriptorHeap(heap);

        for (auto b : outputs)
            if (b)
                core.DestroyBuffer(b);

        for (auto b : {upload, local, readback, timestamps})
            if (b)
                core.DestroyBuffer(b);

        if (queries)
            core.DestroyQueryPool(queries);

        if (acquireSemaphore)
            core.DestroyFence(acquireSemaphore);

        if (releaseSemaphore)
            core.DestroyFence(releaseSemaphore);

        if (fence)
            core.DestroyFence(fence);

        if (command)
            core.DestroyCommandBuffer(command);

        if (allocator)
            core.DestroyCommandAllocator(allocator);
        openrhiDestroyDevice(device);
    }

    Buffer* BufferAt(uint64_t size, MemoryLocation location, BufferUsageBits usage = BufferUsageBits::NONE) {
        Buffer* buffer = nullptr;
        BufferDesc desc = {};
        desc.size = size;
        desc.usage = usage;

        if (usage == BufferUsageBits::SHADER_RESOURCE_STORAGE)
            desc.structureStride = 4;
        REQUIRE(core.CreateCommittedBuffer(*device, location, 0, desc, buffer) == Result::SUCCESS);

        return buffer;
    }

    void Begin() {
        core.ResetCommandAllocator(*allocator);
        REQUIRE(core.BeginCommandBuffer(*command, pool) == Result::SUCCESS);
    }

    void Submit() {
        REQUIRE(core.EndCommandBuffer(*command) == Result::SUCCESS);
        FenceSubmitDesc signal = {fence, ++value, StageBits::ALL};
        QueueSubmitDesc submit = {};
        submit.commandBuffers = &command;
        submit.commandBufferNum = 1;
        submit.signalFences = &signal;
        submit.signalFenceNum = 1;
        REQUIRE(core.QueueSubmit(*queue, submit) == Result::SUCCESS);
        core.Wait(*fence, value);
        REQUIRE(core.GetFenceValue(*fence) >= value);
    }

    void Barrier(Buffer* buffer, AccessBits before, StageBits beforeStage, AccessBits after, StageBits afterStage) {
        BufferBarrierDesc transition = {buffer, {before, beforeStage}, {after, afterStage}};
        BarrierDesc batch = {};
        batch.buffers = &transition;
        batch.bufferNum = 1;
        core.CmdBarrier(*command, batch);
    }

    void Compute(const char* path, bool directHeap = false) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        REQUIRE(file.good());
        auto size = file.tellg();
        REQUIRE(size > 0);
        std::vector<char> code(size_t(size), 0);
        file.seekg(0);
        file.read(code.data(), size);
        DescriptorRangeDesc range = {0, 2, DescriptorType::STORAGE_STRUCTURED_BUFFER, StageBits::COMPUTE_SHADER, DescriptorRangeBits::ARRAY};
        DescriptorSetDesc setDesc = {0, &range, 1, DescriptorSetBits::NONE};
        PipelineLayoutDesc layoutDesc = {};
        layoutDesc.rootRegisterSpace = 1;
        layoutDesc.flags = PipelineLayoutBits::IGNORE_GLOBAL_SPIRV_OFFSETS;
        layoutDesc.descriptorSets = &setDesc;
        layoutDesc.descriptorSetNum = 1;
        layoutDesc.shaderStages = StageBits::COMPUTE_SHADER;
        RootConstantDesc indices = {0, 8, StageBits::COMPUTE_SHADER};

        if (directHeap) {
            layoutDesc.flags |= PipelineLayoutBits::RESOURCE_HEAP_DIRECTLY_INDEXED;
            layoutDesc.descriptorSets = nullptr;
            layoutDesc.descriptorSetNum = 0;
            layoutDesc.rootConstants = &indices;
            layoutDesc.rootConstantNum = 1;
        }

        REQUIRE(core.CreatePipelineLayout(*device, layoutDesc, layout) == Result::SUCCESS);
        ComputePipelineDesc pipelineDesc = {};
        pipelineDesc.pipelineLayout = layout;
        pipelineDesc.shader = {StageBits::COMPUTE_SHADER, code.data(), uint64_t(code.size()), "descriptorMain"};
        pipelineDesc.shader.threadGroupSizeX = 64;
        pipelineDesc.shader.threadGroupSizeY = 1;
        pipelineDesc.shader.threadGroupSizeZ = 1;
        REQUIRE(core.CreateComputePipeline(*device, pipelineDesc, pipeline) == Result::SUCCESS);
        DescriptorPoolDesc poolDesc = {};
        poolDesc.descriptorSetMaxNum = 1;
        poolDesc.storageStructuredBufferMaxNum = 2;
        if (directHeap) {
            REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(DescriptorHeapInterface), &heaps) == Result::SUCCESS);
            REQUIRE(heaps.CreateDescriptorHeap(*device, {64, 0}, heap) == Result::SUCCESS);
        } else {
            REQUIRE(core.CreateDescriptorPool(*device, poolDesc, pool) == Result::SUCCESS);
            REQUIRE(core.AllocateDescriptorSets(*pool, *layout, 0, &set, 1, 0) == Result::SUCCESS);
        }

        for (uint32_t i = 0; i < 2; i++) {
            outputs[i] = BufferAt(2048, MemoryLocation::DEVICE, BufferUsageBits::SHADER_RESOURCE_STORAGE);
            BufferViewDesc view = {};
            view.buffer = outputs[i];
            view.type = BufferView::STORAGE_STRUCTURED_BUFFER;
            view.size = 2048;
            REQUIRE(core.CreateBufferView(view, views[i]) == Result::SUCCESS);
        }
        const uint32_t slots[] = {7, 29};

        if (directHeap) {
            const WriteResourceDescriptorsDesc writes[] = {{views[0], slots[0]}, {views[1], slots[1]}};
            REQUIRE(heaps.WriteResourceDescriptors(*heap, writes, 2) == Result::SUCCESS);
        } else {
            UpdateDescriptorRangeDesc update = {set, 0, 0, views, 2};
            core.UpdateDescriptorRanges(&update, 1);
        }
        Begin();

        for (auto output : outputs)
            Barrier(output, AccessBits::NONE, StageBits::NONE, AccessBits::SHADER_RESOURCE_STORAGE, StageBits::COMPUTE_SHADER);
        if (directHeap)
            heaps.CmdSetDescriptorHeap(*command, *heap);

        core.CmdSetPipelineLayout(*command, BindPoint::COMPUTE, *layout);
        core.CmdSetPipeline(*command, *pipeline);

        if (directHeap) {
            SetRootConstantsDesc constants = {0, slots, sizeof(slots), 0, BindPoint::COMPUTE};
            core.CmdSetRootConstants(*command, constants);
        } else {
            SetDescriptorSetDesc binding = {0, set, BindPoint::COMPUTE};
            core.CmdSetDescriptorSet(*command, binding);
        }
        DispatchDesc dispatch = {16, 1, 1};
        core.CmdDispatch(*command, dispatch);

        for (uint32_t i = 0; i < 2; i++) {
            Barrier(outputs[i], AccessBits::SHADER_RESOURCE_STORAGE, StageBits::COMPUTE_SHADER, AccessBits::COPY_SOURCE, StageBits::COPY);
            core.CmdCopyBuffer(*command, *readback, i * 2048, *outputs[i], 0, 2048);
        }
        Submit();
        const uint32_t* result = (const uint32_t*)core.MapBuffer(*readback, 0, 4096);
        REQUIRE(result);

        for (uint32_t i = 0; i < 1024; i++)
            REQUIRE(result[(i & 1) * 512 + (i >> 1)] == i * 3 + 7);
        core.UnmapBuffer(*readback);
    }

    std::vector<char> ReadShader(const char* path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        REQUIRE(file.good());
        auto size = file.tellg();
        REQUIRE(size > 0);
        std::vector<char> code(size_t(size), 0);
        file.seekg(0);
        file.read(code.data(), size);

        return code;
    }

    void Upscale() {
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(UpscalerInterface), &upscaler) == Result::SUCCESS);
        REQUIRE(upscaler.IsUpscalerSupported(*device, UpscalerType::NIS));
        UpscalerDesc description = {};
        description.type = UpscalerType::NIS;
        description.upscaleResolution = {32, 32};

        for (auto format : {Format::RGBA8_UNORM, Format::RGBA16_SFLOAT, Format::RGBA32_SFLOAT}) {
            for (auto flags : {UpscalerBits::HDR, UpscalerBits::NONE}) {
                description.outputFormat = format;
                description.flags = flags;
                REQUIRE(upscaler.CreateUpscaler(*device, description, nis) == Result::SUCCESS);
                upscaler.DestroyUpscaler(nis);
                nis = nullptr;
            }
        }
        description.outputFormat = Format::RGBA8_UNORM;
        REQUIRE(upscaler.CreateUpscaler(*device, description, nis) == Result::SUCCESS);

        for (uint32_t i = 0; i < 2; i++) {
            TextureDesc texture = {};
            texture.type = TextureType::TEXTURE_2D;
            texture.format = Format::RGBA8_UNORM;
            texture.usage = i ? TextureUsageBits::SHADER_RESOURCE_STORAGE : TextureUsageBits::SHADER_RESOURCE;
            texture.width = texture.height = i ? 32 : 16;
            texture.depth = texture.mipNum = texture.layerNum = texture.sampleNum = 1;
            REQUIRE(core.CreateCommittedTexture(*device, MemoryLocation::DEVICE, 0, texture, nisTextures[i]) == Result::SUCCESS);
            TextureViewDesc view = {};
            view.texture = nisTextures[i];
            view.format = texture.format;
            view.type = i ? TextureView::STORAGE_TEXTURE : TextureView::TEXTURE;
            REQUIRE(core.CreateTextureView(view, nisViews[i]) == Result::SUCCESS);
        }
        std::vector<uint32_t> red(16 * 16, 0xff0000ff);
        TextureSubresourceUploadDesc subresource = {red.data(), 1, 16 * 4, 16 * 16 * 4};
        TextureUploadDesc uploadDescription = {};
        uploadDescription.texture = nisTextures[0];
        uploadDescription.subresources = &subresource;
        uploadDescription.after = {AccessBits::SHADER_RESOURCE, Layout::SHADER_RESOURCE, StageBits::COMPUTE_SHADER};
        HelperInterface helper = {};
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(HelperInterface), &helper) == Result::SUCCESS);
        REQUIRE(helper.UploadData(*queue, &uploadDescription, 1, nullptr, 0) == Result::SUCCESS);
        const uint32_t rowPitch = std::max(128u, core.GetDeviceDesc(*device).memoryAlignment.uploadBufferTextureRow);
        nisPixels = BufferAt(uint64_t(rowPitch) * 32, MemoryLocation::HOST_READBACK);
        Begin();
        TextureBarrierDesc transition = {};
        transition.texture = nisTextures[1];
        transition.after = {AccessBits::SHADER_RESOURCE_STORAGE, Layout::SHADER_RESOURCE_STORAGE, StageBits::COMPUTE_SHADER};
        BarrierDesc barriers = {};
        barriers.textures = &transition;
        barriers.textureNum = 1;
        core.CmdBarrier(*command, barriers);
        DispatchUpscaleDesc dispatch = {};
        dispatch.input = {nisTextures[0], nisViews[0]};
        dispatch.output = {nisTextures[1], nisViews[1]};
        dispatch.currentResolution = {16, 16};
        dispatch.settings.nis.sharpness = 0.5f;
        upscaler.CmdDispatchUpscale(*command, *nis, dispatch);
        transition.before = transition.after;
        transition.after = {AccessBits::COPY_SOURCE, Layout::COPY_SOURCE, StageBits::COPY};
        core.CmdBarrier(*command, barriers);
        TextureDataLayoutDesc data = {0, rowPitch, rowPitch * 32};
        TextureRegionDesc region = {};
        region.width = region.height = 32;
        region.depth = 1;
        core.CmdReadbackTextureToBuffer(*command, *nisPixels, data, *nisTextures[1], region);
        Submit();
        const uint8_t* result = (const uint8_t*)core.MapBuffer(*nisPixels, 0, uint64_t(rowPitch) * 32);
        REQUIRE(result);

        for (uint32_t y = 0; y < 32; y++)
            for (uint32_t x = 0; x < 32; x++) {
                const uint8_t* pixel = result + y * rowPitch + x * 4;

                if (!(pixel[0] >= 250 && pixel[1] <= 4 && pixel[2] <= 4))
                    std::fprintf(stderr, "NIS pixel %u,%u = %u,%u,%u,%u\n", x, y, pixel[0], pixel[1], pixel[2], pixel[3]);
                REQUIRE(pixel[0] >= 250 && pixel[1] <= 4 && pixel[2] <= 4);
            }
        core.UnmapBuffer(*nisPixels);
    }

    void Render(const char* vertex, const char* fragment, const char* task = nullptr) {
        auto vs = ReadShader(vertex), ps = ReadShader(fragment);
        auto taskCode = task ? ReadShader(task) : std::vector<char>{};
        TextureDesc texture = {};
        texture.type = TextureType::TEXTURE_2D;
        texture.usage = TextureUsageBits::COLOR_ATTACHMENT;
        texture.format = Format::RGBA8_UNORM;
        texture.width = texture.height = 32;
        texture.depth = texture.mipNum = texture.layerNum = texture.sampleNum = 1;
        REQUIRE(core.CreateCommittedTexture(*device, MemoryLocation::DEVICE, 0, texture, color) == Result::SUCCESS);
        TextureViewDesc view = {};
        view.texture = color;
        view.type = TextureView::COLOR_ATTACHMENT;
        view.format = texture.format;
        REQUIRE(core.CreateTextureView(view, colorView) == Result::SUCCESS);
        PipelineLayoutDesc layoutDesc = {};
        layoutDesc.shaderStages = (task ? StageBits::MESH_SHADERS : StageBits::VERTEX_SHADER) | StageBits::FRAGMENT_SHADER;
        REQUIRE(core.CreatePipelineLayout(*device, layoutDesc, graphicsLayout) == Result::SUCCESS);
        ShaderDesc shaders[] = {
            {task ? StageBits::MESH_SHADER : StageBits::VERTEX_SHADER, vs.data(), uint64_t(vs.size()), task ? "meshMain" : "vsMain", 1, 1, 1, task ? 4u : 0u},
            {StageBits::FRAGMENT_SHADER, ps.data(), uint64_t(ps.size()), "psMain"},
            {StageBits::TASK_SHADER, taskCode.data(), uint64_t(taskCode.size()), "taskMain", 1, 1, 1, 4},
        };
        ColorAttachmentDesc attachment = {};
        attachment.format = texture.format;
        attachment.colorWriteMask = ColorWriteBits::RGBA;
        GraphicsPipelineDesc pipelineDesc = {};
        pipelineDesc.pipelineLayout = graphicsLayout;
        pipelineDesc.inputAssembly.topology = Topology::TRIANGLE_LIST;
        pipelineDesc.rasterization.cullMode = CullMode::NONE;
        pipelineDesc.outputMerger.colors = &attachment;
        pipelineDesc.outputMerger.colorNum = 1;
        pipelineDesc.shaders = shaders;
        pipelineDesc.shaderNum = task ? 3 : 2;
        REQUIRE(core.CreateGraphicsPipeline(*device, pipelineDesc, graphicsPipeline) == Result::SUCCESS);
        arguments = BufferAt(sizeof(DrawDesc), MemoryLocation::HOST_UPLOAD, BufferUsageBits::ARGUMENT);
        DrawDesc draw = {3, 1, 0, 0};
        void* mapped = core.MapBuffer(*arguments, 0, sizeof(draw));
        REQUIRE(mapped);
        std::memcpy(mapped, &draw, sizeof(draw));
        core.UnmapBuffer(*arguments);
        uint32_t rowPitch = core.GetDeviceDesc(*device).memoryAlignment.uploadBufferTextureRow;

        if (rowPitch < 128)
            rowPitch = 128;
        pixels = BufferAt(uint64_t(rowPitch) * 32, MemoryLocation::HOST_READBACK);
        Begin();
        TextureBarrierDesc transition = {};
        transition.texture = color;
        transition.after = {AccessBits::COLOR_ATTACHMENT, Layout::COLOR_ATTACHMENT, StageBits::COLOR_ATTACHMENT};
        BarrierDesc barriers = {};
        barriers.textures = &transition;
        barriers.textureNum = 1;
        core.CmdBarrier(*command, barriers);
        core.CmdSetPipelineLayout(*command, BindPoint::GRAPHICS, *graphicsLayout);
        core.CmdSetPipeline(*command, *graphicsPipeline);
        Viewport viewport = {0, 0, 32, 32, 0, 1, false};
        Rect scissor = {0, 0, 32, 32};
        core.CmdSetViewports(*command, &viewport, 1);
        core.CmdSetScissors(*command, &scissor, 1);
        AttachmentDesc target = {};
        target.descriptor = colorView;
        target.loadOp = LoadOp::CLEAR;
        target.storeOp = StoreOp::STORE;
        RenderingDesc rendering = {};
        rendering.colors = &target;
        rendering.colorNum = 1;
        core.CmdBeginRendering(*command, rendering);

        if (task) {
            MeshShaderInterface mesh = {};
            REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(MeshShaderInterface), &mesh) == Result::SUCCESS);
            DrawMeshTasksDesc drawMesh = {1, 1, 1};
            mesh.CmdDrawMeshTasks(*command, drawMesh);
        } else
            core.CmdDrawIndirect(*command, *arguments, 0, 1, sizeof(DrawDesc), nullptr, 0);
        ClearAttachmentDesc clear = {};
        clear.planes = PlaneBits::COLOR;
        clear.value.color.f = {0, 1, 0, 1};
        Rect clearRect = {8, 8, 8, 8};
        core.CmdClearAttachments(*command, &clear, 1, &clearRect, 1);
        core.CmdEndRendering(*command);
        transition.before = transition.after;
        transition.after = {AccessBits::COPY_SOURCE, Layout::COPY_SOURCE, StageBits::COPY};
        core.CmdBarrier(*command, barriers);
        TextureDataLayoutDesc data = {0, rowPitch, rowPitch * 32};
        TextureRegionDesc region = {};
        region.width = region.height = 32;
        region.depth = 1;
        core.CmdReadbackTextureToBuffer(*command, *pixels, data, *color, region);
        Submit();
        const uint8_t* result = (const uint8_t*)core.MapBuffer(*pixels, 0, uint64_t(rowPitch) * 32);
        REQUIRE(result);

        for (uint32_t y = 0; y < 32; y++)
            for (uint32_t x = 0; x < 32; x++) {
                const uint8_t* pixel = result + y * rowPitch + x * 4;
                const bool cleared = x >= 8 && x < 16 && y >= 8 && y < 16;
                REQUIRE(pixel[0] == (cleared ? 0 : 255) && pixel[1] == (cleared ? 255 : 0) && pixel[2] == 0 && pixel[3] == 255);
            }
        core.UnmapBuffer(*pixels);
    }
#if defined(_WIN32)
    void Present() {
        struct WindowOwner {
            HWND handle = CreateWindowExW(0, L"STATIC", L"OpenRHI qualification", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);

            ~WindowOwner() {
                if (handle)
                    DestroyWindow(handle);
            }
        } window;

        REQUIRE(window.handle);
        SwapChainInterface swap = {};
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(SwapChainInterface), &swap) == Result::SUCCESS);
        SwapChainDesc desc = {};
        desc.window.windows.hwnd = window.handle;
        desc.queue = queue;
        desc.width = desc.height = 32;
        desc.textureNum = 2;
        desc.format = SwapChainFormat::BT709_G22_8BIT;
        SwapChain* chain = nullptr;
        REQUIRE(swap.CreateSwapChain(*device, desc, chain) == Result::SUCCESS);

        struct ChainOwner {
            SwapChainInterface& api;
            SwapChain* handle;

            ~ChainOwner() {
                api.DestroySwapChain(handle);
            }
        } chainOwner{swap, chain};

        uint32_t count = 0;
        Texture* const* textures = swap.GetSwapChainTextures(*chain, count);
        REQUIRE(textures && count >= 2);
        REQUIRE(core.CreateFence(*device, SWAPCHAIN_SEMAPHORE, acquireSemaphore) == Result::SUCCESS);
        REQUIRE(core.CreateFence(*device, SWAPCHAIN_SEMAPHORE, releaseSemaphore) == Result::SUCCESS);
        uint32_t index = 0;
        REQUIRE(swap.AcquireNextTexture(*chain, *acquireSemaphore, index) == Result::SUCCESS && index < count);
        Begin();
        // The untouched D3D12 swapchain image is in PRESENT/COMMON state
        Submit();
        REQUIRE(swap.QueuePresent(*chain, *releaseSemaphore, 0) == Result::SUCCESS);
        REQUIRE(core.DeviceWaitIdle(device) == Result::SUCCESS);
    }
#endif
};

int main(int argc, char** argv) {
    try {
        GraphicsAPI api = GraphicsAPI::D3D12;
        bool direct = false, nativeValidation = false;
        uint64_t bytes = 4 * 1024 * 1024;
        uint32_t iterations = 64;
        double minSeconds = 0;
        AdapterDesc selectedAdapter = {};
        bool explicitAdapter = false;
        const char* shader = nullptr;
        bool directHeap = false;
        const char* vertex = nullptr;
        const char* fragment = nullptr;
        bool present = false;
        bool testNis = false;
        bool testMesh = false;
        const char* task = nullptr;

        for (int i = 1; i < argc; i++) {
            const std::string arg = argv[i];

            if (arg == "--api" && i + 1 < argc) {
                std::string name = argv[++i];
                api = name == "VK" ? GraphicsAPI::VK : (name == "METAL" ? GraphicsAPI::METAL : GraphicsAPI::D3D12);
            } else if (arg == "--variant" && i + 1 < argc)
                direct = std::string(argv[++i]) == "device-upload";
            else if (arg == "--bytes" && i + 1 < argc)
                bytes = std::stoull(argv[++i]);
            else if (arg == "--iterations" && i + 1 < argc)
                iterations = uint32_t(std::stoul(argv[++i]));
            else if (arg == "--seconds" && i + 1 < argc)
                minSeconds = std::stod(argv[++i]);
            else if (arg == "--uid-low" && i + 1 < argc) {
                selectedAdapter.uid.low = std::stoull(argv[++i], nullptr, 0);
                explicitAdapter = true;
            } else if (arg == "--uid-high" && i + 1 < argc) {
                selectedAdapter.uid.high = std::stoull(argv[++i], nullptr, 0);
                explicitAdapter = true;
            } else if (arg == "--shader" && i + 1 < argc)
                shader = argv[++i];
            else if (arg == "--heap-shader" && i + 1 < argc) {
                shader = argv[++i];
                directHeap = true;
            } else if (arg == "--vs" && i + 1 < argc)
                vertex = argv[++i];
            else if (arg == "--ps" && i + 1 < argc)
                fragment = argv[++i];
            else if (arg == "--task" && i + 1 < argc)
                task = argv[++i];
            else if (arg == "--mesh")
                testMesh = true;
            else if (arg == "--nis")
                testNis = true;
            else if (arg == "--present")
                present = true;
            else if (arg == "--native-validation")
                nativeValidation = true;
            else
                throw std::runtime_error("Unknown or incomplete argument");
        }
        REQUIRE(bytes >= 4096 && bytes <= 256ull * 1024 * 1024 && bytes % 4 == 0 && iterations > 0);
        REQUIRE(minSeconds >= 0 && minSeconds <= 86400);

        if (explicitAdapter) {
            uint32_t count = 0;
            REQUIRE(openrhiEnumerateAdapters(nullptr, count) == Result::SUCCESS);
            std::vector<AdapterDesc> adapters(count);
            REQUIRE(openrhiEnumerateAdapters(adapters.data(), count) == Result::SUCCESS);
            bool found = false;

            for (const auto& adapter : adapters) {
                if (adapter.uid.low == selectedAdapter.uid.low && adapter.uid.high == selectedAdapter.uid.high) {
                    selectedAdapter = adapter;
                    found = true;
                    break;
                }
            }
            REQUIRE(found);
        }
        Fixture f;
        DeviceCreationDesc creation = {};
        DeviceCompatibilityReport compatibility = {};
        creation.graphicsAPI = api;
        creation.adapterDesc = explicitAdapter ? &selectedAdapter : nullptr;
        creation.enableOpenRHIValidation = true;
        creation.enableGraphicsAPIValidation = nativeValidation;
        creation.callbackInterface = {MessageCallback, NoAbort, nullptr};
        creation.compatibilityReport = &compatibility;
        creation.profilePolicy = ProfilePolicy::GENERIC;
        const Result created = openrhiCreateDevice(creation, f.device);

        if (created == Result::UNSUPPORTED)
            return 77;
        REQUIRE(created == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*f.device, OPENRHI_INTERFACE(CoreInterface), &f.core) == Result::SUCCESS);
        const auto& deviceDesc = f.core.GetDeviceDesc(*f.device);

        if (testMesh && !deviceDesc.features.meshShader)
            return 77;

        if (direct && !deviceDesc.memory.deviceUploadHeapSize) {
            std::fprintf(stderr, "SKIP: GPU upload heap unavailable\n");

            return 77;
        }
        REQUIRE(f.core.GetQueue(*f.device, QueueType::GRAPHICS, 0, f.queue) == Result::SUCCESS);
        REQUIRE(f.core.CreateCommandAllocator(*f.queue, f.allocator) == Result::SUCCESS);
        REQUIRE(f.core.CreateCommandBuffer(*f.allocator, f.command) == Result::SUCCESS);
        REQUIRE(f.core.CreateFence(*f.device, 0, f.fence) == Result::SUCCESS);
        f.upload = f.BufferAt(bytes, direct ? MemoryLocation::DEVICE_UPLOAD : MemoryLocation::HOST_UPLOAD);

        if (!direct)
            f.local = f.BufferAt(bytes, MemoryLocation::DEVICE);
        f.readback = f.BufferAt(bytes, MemoryLocation::HOST_READBACK);
        f.timestamps = f.BufferAt(16, MemoryLocation::HOST_READBACK);
        REQUIRE(deviceDesc.features.timestamp);
        QueryPoolDesc queries = {QueryType::TIMESTAMP, 2};
        REQUIRE(f.core.CreateQueryPool(*f.device, queries, f.queries) == Result::SUCCESS);
        std::vector<uint32_t> source(size_t(bytes / 4));

        for (uint32_t i = 0; i < source.size(); i++)
            source[i] = i * 1664525u + 1013904223u;
        std::vector<double> cpu, gpu;
        const auto workloadStart = Clock::now();

        for (uint64_t i = 0; i < uint64_t(iterations) + 8 || std::chrono::duration<double>(Clock::now() - workloadStart).count() < minSeconds; i++) {
            auto begin = Clock::now();
            void* mapped = f.core.MapBuffer(*f.upload, 0, bytes);
            REQUIRE(mapped);
            std::memcpy(mapped, source.data(), size_t(bytes));
            f.core.UnmapBuffer(*f.upload);
            f.Begin();
            f.core.CmdResetQueries(*f.command, *f.queries, 0, 2);
            f.core.CmdEndQuery(*f.command, *f.queries, 0);

            if (!direct) {
                f.Barrier(f.local, i ? AccessBits::COPY_SOURCE : AccessBits::NONE, i ? StageBits::COPY : StageBits::NONE, AccessBits::COPY_DESTINATION, StageBits::COPY);
                f.core.CmdCopyBuffer(*f.command, *f.local, 0, *f.upload, 0, bytes);
                f.Barrier(f.local, AccessBits::COPY_DESTINATION, StageBits::COPY, AccessBits::COPY_SOURCE, StageBits::COPY);
            }
            f.core.CmdCopyBuffer(*f.command, *f.readback, 0, *(direct ? f.upload : f.local), 0, bytes);
            f.core.CmdEndQuery(*f.command, *f.queries, 1);
            f.core.CmdCopyQueries(*f.command, *f.queries, 0, 2, *f.timestamps, 0);
            f.Submit();
            double elapsed = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
            const void* output = f.core.MapBuffer(*f.readback, 0, bytes);
            REQUIRE(output);
            REQUIRE(std::memcmp(output, source.data(), size_t(bytes)) == 0);
            f.core.UnmapBuffer(*f.readback);
            auto ticks = (const uint64_t*)f.core.MapBuffer(*f.timestamps, 0, 16);
            REQUIRE(ticks);
            REQUIRE(ticks[1] >= ticks[0]);
            double duration = double(ticks[1] - ticks[0]) * 1000.0 / double(deviceDesc.other.timestampFrequencyHz);
            f.core.UnmapBuffer(*f.timestamps);

            if (i >= 8) {
                cpu.push_back(elapsed);
                gpu.push_back(duration);
            }
        }

        const auto& bindlessCaps = f.core.GetDeviceDesc(*f.device);

        if (directHeap) {
            if (!bindlessCaps.features.descriptorHeap)
                return 77;

            std::printf("Bindless heap caps: shaderModel=%u tier=%u resourceBinding=%u resources=%u samplers=%u rootBytes=%u\n", unsigned(bindlessCaps.shaderModel), bindlessCaps.tiers.bindless, bindlessCaps.tiers.resourceBinding, bindlessCaps.descriptorHeap.resourceMaxNum, bindlessCaps.descriptorHeap.samplerMaxNum, bindlessCaps.descriptorHeap.rootConstantMaxSize);
        }

        if (shader)
            f.Compute(shader, directHeap);

        if (testNis)
            f.Upscale();

        if (vertex && fragment)
            f.Render(vertex, fragment, testMesh ? task : nullptr);
#if defined(_WIN32)

        if (present)
            f.Present();
#else
        REQUIRE(!present);
#endif
        REQUIRE(validationErrors == 0);
        HelperInterface helper = {};
        VideoMemoryInfo memory = {};
        const bool memoryKnown = openrhiGetInterface(*f.device, OPENRHI_INTERFACE(HelperInterface), &helper) == Result::SUCCESS && helper.QueryVideoMemoryInfo(*f.device, MemoryLocation::DEVICE, memory) == Result::SUCCESS;
        const auto& adapter = deviceDesc.adapterDesc;
        std::printf("{\"schema\":1,\"api\":%s,\"device\":%s,\"vendor_id\":%u,\"device_id\":%u,\"revision_id\":%u,\"driver\":%llu,\"uid_low\":%llu,\"uid_high\":%llu,\"family\":%u,\"profile_revision\":%u,\"tuned\":%s,\"variant\":%s,\"bytes\":%llu,\"memory_budget\":%llu,\"validation_errors\":%u,\"native_validation\":%s,\"descriptor_compute_checked\":%s,\"cpu_ms\":[", Quote(openrhiGetGraphicsAPIString(api)).c_str(), Quote(adapter.name).c_str(), adapter.vendorId, adapter.deviceId, adapter.revisionId, (unsigned long long)adapter.nativeDriverVersion, (unsigned long long)adapter.uid.low, (unsigned long long)adapter.uid.high, uint32_t(openrhiGetDeviceProfile(*f.device)->detectedFamily), openrhiGetDeviceProfile(*f.device)->revision, openrhiGetDeviceProfile(*f.device)->tunedPoliciesEnabled ? "true" : "false", Quote(direct ? "device-upload" : "staging").c_str(), (unsigned long long)bytes, (unsigned long long)adapter.memoryBudget, validationErrors, nativeValidation ? "true" : "false", shader ? "true" : "false");

        for (size_t i = 0; i < cpu.size(); i++)
            std::printf("%s%.9f", i ? "," : "", cpu[i]);
        std::printf("],\"bindless_heap_checked\":%s,\"gpu_ms\":[", directHeap ? "true" : "false");

        for (size_t i = 0; i < gpu.size(); i++)
            std::printf("%s%.9f", i ? "," : "", gpu[i]);
        std::printf("],\"memory_usage\":%llu,\"memory_usage_known\":%s", (unsigned long long)memory.usageSize, memoryKnown ? "true" : "false");
        std::printf(",\"sustained_seconds\":%.6f,\"indirect_draw_checked\":%s,\"presentation_checked\":%s,\"memory_budget_known\":%s,\"revision_id_known\":%s,\"capabilities\":%u}\n", std::chrono::duration<double>(Clock::now() - workloadStart).count(), vertex && fragment ? "true" : "false", present ? "true" : "false", adapter.memoryBudgetKnown ? "true" : "false", adapter.revisionIdKnown ? "true" : "false", uint32_t(deviceDesc.nativeCapabilities));

        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Qualification failed: %s\n", error.what());

        return 1;
    }
}
