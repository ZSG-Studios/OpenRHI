#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#endif

#include "OpenRHI.h"

#if defined(_WIN32)
#    include "OpenRHIAgilitySDK.h"
#endif

#include "Extensions/OpenRHIDescriptorHeap.h"
#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIHelper.h"
#include "Reference.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numeric>
#include <string>

using namespace openrhi;
using namespace material_test;
static std::atomic<uint32_t> errors{0};

static void OPENRHI_CALL MessageCallback(Message type, const char*, uint32_t, const char* message, void*) {
    if (type == Message::ERROR)
        errors++;
    std::fprintf(stderr, "%s\n", message);
}

static void OPENRHI_CALL NoAbort(void*) {
}

static void Success(Result result, const char* operation) {
    if (result != Result::SUCCESS)
        throw std::runtime_error(std::string(operation) + ": result=" + std::to_string(uint32_t(result)));
}

static std::vector<char> Read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    Check(file.good() && file.tellg() > 0, "missing shader");
    std::vector<char> bytes(size_t(file.tellg()));
    file.seekg(0);
    file.read(bytes.data(), std::streamsize(bytes.size()));
    Check(file.good(), "shader read");

    return bytes;
}

static void Write(const std::filesystem::path& path, const void* data, size_t size) {
    std::ofstream file(path, std::ios::binary);
    file.write((const char*)data, std::streamsize(size));
    Check(file.good(), "artifact write");
}

static std::string Quoted(const std::string& value) {
    std::string result = "\"";

    for (char c : value) {
        if (c == '\\' || c == '"')
            result += '\\';
        Check(uint8_t(c) >= 32, "JSON control character");
        result += c;
    }

    return result + '"';
}

struct Fixture {
    Device* device = nullptr;
    CoreInterface core = {};
    HelperInterface helper = {};
    DescriptorHeapInterface heapApi = {};
    Queue* queue = nullptr;
    CommandAllocator* allocator = nullptr;
    CommandBuffer* command = nullptr;
    Fence* fence = nullptr;
    DescriptorHeap* heap = nullptr;
    PipelineLayout* layouts[2] = {};
    Pipeline* pipelines[2] = {};
    Descriptor* samplers[4] = {};
    SamplerDesc samplerDescriptions[4] = {};
    uint64_t completion = 0;
    uint32_t destroyedOwners = 0;
    uint32_t anisotropy = 1;

    ~Fixture() {
        if (!device)
            return;

        if (core.DeviceWaitIdle)
            core.DeviceWaitIdle(device);

        for (auto sampler : samplers)
            if (sampler)
                core.DestroyDescriptor(sampler);

        for (auto pipeline : pipelines)
            if (pipeline)
                core.DestroyPipeline(pipeline);

        for (auto layout : layouts)
            if (layout)
                core.DestroyPipelineLayout(layout);

        if (heap)
            heapApi.DestroyDescriptorHeap(heap);

        if (fence)
            core.DestroyFence(fence);

        if (command)
            core.DestroyCommandBuffer(command);

        if (allocator)
            core.DestroyCommandAllocator(allocator);
        openrhiDestroyDevice(device);
    }

    void Create(const char* directShader, const char* conventionalShader, uint32_t maximumAnisotropy = 1) {
        anisotropy = maximumAnisotropy;
        DeviceCreationDesc desc = {};
        desc.graphicsAPI = GraphicsAPI::D3D12;
        desc.enableOpenRHIValidation = desc.enableGraphicsAPIValidation = true;
        desc.profilePolicy = ProfilePolicy::GENERIC;
        desc.callbackInterface = {MessageCallback, NoAbort, nullptr};
        Success(openrhiCreateDevice(desc, device), "create device");
        Success(openrhiGetInterface(*device, OPENRHI_INTERFACE(CoreInterface), &core), "core");
        Success(openrhiGetInterface(*device, OPENRHI_INTERFACE(HelperInterface), &helper), "helper");
        Success(openrhiGetInterface(*device, OPENRHI_INTERFACE(DescriptorHeapInterface), &heapApi), "heap interface");
        const auto& caps = core.GetDeviceDesc(*device);
        Check(caps.features.descriptorHeap && caps.features.textureCompressionBC, "heap and BC capabilities");
        Check(caps.other.samplerAnisotropyMax >= anisotropy, "regular sampler anisotropy limit");
        Check(caps.descriptorHeap.resourceMaxNum >= 256 && caps.descriptorHeap.samplerMaxNum >= 64 && caps.descriptorHeap.rootConstantMaxSize >= 16, "heap limits");
        Success(core.GetQueue(*device, QueueType::GRAPHICS, 0, queue), "queue");
        Success(core.CreateCommandAllocator(*queue, allocator), "allocator");
        Success(core.CreateCommandBuffer(*allocator, command), "command buffer");
        Success(core.CreateFence(*device, 0, fence), "fence");
        Success(heapApi.CreateDescriptorHeap(*device, {256, 64}, heap), "heap");

        for (uint32_t i = 0; i < 4; i++) {
            SamplerDesc sampler = {};
            sampler.filters.min = sampler.filters.mag = sampler.filters.mip = (anisotropy == 16 || (i & 2)) ? Filter::LINEAR : Filter::NEAREST;
            sampler.addressModes.u = sampler.addressModes.v = sampler.addressModes.w = (i & 1) ? AddressMode::REPEAT : AddressMode::CLAMP_TO_EDGE;
            sampler.anisotropy = uint8_t(anisotropy);
            sampler.mipMax = 32;
            Success(core.CreateSampler(*device, sampler, samplers[i]), "sampler");
            samplerDescriptions[i] = sampler;
            WriteSamplerDescriptorsDesc write = {samplers[i], 7 + i * 12};
            Success(heapApi.WriteSamplerDescriptors(*heap, &write, 1), "sampler heap write");
        }

        for (uint32_t i = 0; i < 2; i++) {
            RootConstantDesc constants = {0, 16, StageBits::COMPUTE_SHADER};
            DescriptorRangeDesc ranges[] = {
                {0, 1, DescriptorType::STRUCTURED_BUFFER, StageBits::COMPUTE_SHADER},
                {1, 1, DescriptorType::STORAGE_STRUCTURED_BUFFER, StageBits::COMPUTE_SHADER},
                {2, 1, DescriptorType::TEXTURE, StageBits::COMPUTE_SHADER},
                {3, 1, DescriptorType::SAMPLER, StageBits::COMPUTE_SHADER},
            };
            DescriptorSetDesc set = {};
            set.ranges = ranges;
            set.rangeNum = 4;
            PipelineLayoutDesc layout = {};
            layout.rootRegisterSpace = 1;
            layout.rootConstants = &constants;
            layout.rootConstantNum = 1;
            layout.shaderStages = StageBits::COMPUTE_SHADER;
            layout.flags = i ? PipelineLayoutBits::IGNORE_GLOBAL_SPIRV_OFFSETS : (PipelineLayoutBits::RESOURCE_HEAP_DIRECTLY_INDEXED | PipelineLayoutBits::SAMPLER_HEAP_DIRECTLY_INDEXED);
            layout.descriptorSets = i ? &set : nullptr;
            layout.descriptorSetNum = i;
            Success(core.CreatePipelineLayout(*device, layout, layouts[i]), "pipeline layout");
            const auto code = Read(i ? conventionalShader : directShader);
            ComputePipelineDesc compute = {};
            compute.pipelineLayout = layouts[i];
            compute.shader = {StageBits::COMPUTE_SHADER, code.data(), uint64_t(code.size()), "materialMain", 64, 1, 1};
            Success(core.CreateComputePipeline(*device, compute, pipelines[i]), "compute pipeline");
        }
    }

    Buffer* MakeBuffer(uint64_t bytes, uint32_t stride, MemoryLocation location, BufferUsageBits usage) {
        BufferDesc desc = {};
        desc.size = bytes;
        desc.structureStride = stride;
        desc.usage = usage;
        Buffer* buffer = nullptr;
        Success(core.CreateCommittedBuffer(*device, location, 0, desc, buffer), "buffer");

        return buffer;
    }

    Descriptor* MakeBufferView(Buffer* buffer, bool storage) {
        BufferViewDesc desc = {};
        desc.buffer = buffer;
        desc.type = storage ? BufferView::STORAGE_STRUCTURED_BUFFER : BufferView::STRUCTURED_BUFFER;
        Descriptor* view = nullptr;
        Success(core.CreateBufferView(desc, view), "buffer view");

        return view;
    }

    void BufferBarrier(Buffer* buffer, AccessBits before, StageBits beforeStage, AccessBits after, StageBits afterStage) {
        BufferBarrierDesc transition = {buffer, {before, beforeStage}, {after, afterStage}};
        BarrierDesc barriers = {};
        barriers.buffers = &transition;
        barriers.bufferNum = 1;
        core.CmdBarrier(*command, barriers);
    }
};

struct TextureOwner {
    Fixture& fixture;
    Texture* texture = nullptr;
    std::vector<Descriptor*> views;

    explicit TextureOwner(Fixture& f) : fixture(f) {
    }

    ~TextureOwner() {
        for (auto view : views)
            fixture.core.DestroyDescriptor(view);

        if (texture)
            fixture.core.DestroyTexture(texture);
        fixture.destroyedOwners++;
    }
};

struct Batch {
    Fixture& fixture;
    DescriptorPool* pool = nullptr;
    Buffer* input = nullptr;
    Buffer* output[2] = {};
    Buffer* readback[2] = {};
    Descriptor* inputView = nullptr;
    Descriptor* outputViews[2] = {};
    std::vector<std::shared_ptr<TextureOwner>> recordingOwners;
    std::vector<std::shared_ptr<TextureOwner>> retiredOwners;
    bool submitted = false;

    explicit Batch(Fixture& f) : fixture(f) {
    }

    ~Batch() {
        // Failure cleanup also preserves native owners until the queue is idle
        if (submitted)
            fixture.core.DeviceWaitIdle(fixture.device);
        recordingOwners.clear();
        retiredOwners.clear();

        if (pool)
            fixture.core.DestroyDescriptorPool(pool);

        for (auto view : outputViews)
            if (view)
                fixture.core.DestroyDescriptor(view);

        if (inputView)
            fixture.core.DestroyDescriptor(inputView);

        for (auto buffer : output)
            if (buffer)
                fixture.core.DestroyBuffer(buffer);

        for (auto buffer : readback)
            if (buffer)
                fixture.core.DestroyBuffer(buffer);

        if (input)
            fixture.core.DestroyBuffer(input);
    }
};

static Format NativeFormat(Encoding encoding) {
    switch (encoding) {
        case Encoding::BC4:
            return Format::BC4_R_UNORM;
        case Encoding::BC5:
            return Format::BC5_RG_UNORM;
        case Encoding::BC7:
            return Format::BC7_RGBA_UNORM;
        default:
            return Format::BC7_RGBA_SRGB;
    }
}

struct Group {
    uint32_t first, count;
    Descriptor* texture;
    uint32_t sampler;
    DescriptorSet* set = nullptr;
};

struct Expected {
    material_test::Color value;
    material_test::Color decodeBound;
    uint32_t width, height, levels, sampler;
};

static std::string ProfileName(Encoding encoding, uint32_t width, uint32_t height, uint32_t epoch, uint32_t anisotropy, bool constant) {
    const auto base = std::to_string(uint32_t(encoding)) + "-" + std::to_string(width) + "x" + std::to_string(height) + "-" + std::to_string(epoch);

    return anisotropy == 1 ? base : base + (constant ? "-a16-constant" : "-a16-pattern");
}

static void RunProfile(Fixture& f, Encoding encoding, uint32_t width, uint32_t height, uint32_t epoch, const std::filesystem::path& artifacts, bool constant = false) {
    const uint32_t firstValidationErrors = errors.load();
    Batch batch(f);
    const auto format = NativeFormat(encoding);
    Check((f.core.GetFormatSupport(*f.device, format) & FormatSupportBits::TEXTURE) != 0, "sampled format support");
    const auto prefix = ProfileName(encoding, width, height, epoch, f.anisotropy, constant);
    Image images[] = {constant ? MakeConstantImage(encoding, width, height, 3 + epoch * 17) : MakeImage(encoding, width, height, 3 + epoch * 17), constant ? MakeConstantImage(encoding, width, height, 9 + epoch * 17) : MakeImage(encoding, width, height, 9 + epoch * 17)};
    std::vector<SampleCase> cases;
    std::vector<Expected> expected;
    std::vector<Group> groups;
    const uint32_t baseMips[] = {0, 1, Levels(width, height) - 1};

    for (uint32_t imageIndex = 0; imageIndex < 2; imageIndex++) {
        auto owner = std::make_shared<TextureOwner>(f);
        TextureDesc texture = {};
        texture.type = TextureType::TEXTURE_2D;
        texture.format = format;
        texture.usage = TextureUsageBits::SHADER_RESOURCE;
        texture.width = Dim_t(width);
        texture.height = Dim_t(height);
        texture.depth = texture.layerNum = texture.sampleNum = 1;
        texture.mipNum = Dim_t(images[imageIndex].mips.size());
        Success(f.core.CreateCommittedTexture(*f.device, MemoryLocation::DEVICE, 0, texture, owner->texture), "exact BC original texture factory");
        std::vector<TextureSubresourceUploadDesc> data;

        for (uint32_t mip = 0; mip < images[imageIndex].mips.size(); mip++) {
            const auto& level = images[imageIndex].mips[mip];
            data.push_back({level.bytes.data(), 1, level.rowPitch, uint32_t(level.bytes.size())});
            Write(artifacts / (prefix + "-image" + std::to_string(imageIndex) + "-mip" + std::to_string(mip) + ".bc"), level.bytes.data(), level.bytes.size());
        }
        TextureUploadDesc upload = {};
        upload.texture = owner->texture;
        upload.subresources = data.data();
        upload.after = {AccessBits::SHADER_RESOURCE, Layout::SHADER_RESOURCE, StageBits::COMPUTE_SHADER};
        Success(f.helper.UploadData(*f.queue, &upload, 1, nullptr, 0), "compressed mip upload");

        for (uint32_t viewIndex = 0; viewIndex < 3; viewIndex++) {
            const auto view = Normalize(width, height, uint32_t(images[imageIndex].mips.size()), baseMips[viewIndex], viewIndex == 1 ? 2 : 0);
            TextureViewDesc viewDesc = {};
            viewDesc.texture = owner->texture;
            viewDesc.type = TextureView::TEXTURE;
            viewDesc.format = format;
            viewDesc.mipOffset = Dim_t(view.base);
            viewDesc.mipNum = viewIndex == 1 ? Dim_t(view.count) : 0;
            viewDesc.layout = Layout::SHADER_RESOURCE;
            Descriptor* descriptor = nullptr;
            Success(f.core.CreateTextureView(viewDesc, descriptor), "normalized exact BC sampled view");
            owner->views.push_back(descriptor);
            const uint32_t textureSlot = 5 + imageIndex * 76 + viewIndex * 19;
            const WriteResourceDescriptorsDesc write = {descriptor, textureSlot};
            Success(f.heapApi.WriteResourceDescriptors(*f.heap, &write, 1), "texture heap write");

            for (uint32_t sampler = f.anisotropy == 16 ? 2 : 0; sampler < 4; sampler++) {
                Group group = {uint32_t(cases.size()), 0, descriptor, sampler};
                const float coordinates[][2] = {{.5f, .5f}, {.137f, .313f}, {-.17f, .23f}, {1.19f, .71f}, {.003f, .997f}, {-.73f, 1.43f}, {.8125f, .1875f}, {1.5f, -.5f}};

                for (const auto& uv : coordinates) {
                    for (uint32_t operation = 0; operation < 3; operation++) {
                        const float lods[] = {-.7f, .27f, 1.23f, 3.1f, 9.2f};
                        const uint32_t variants = operation == 0 ? 1 : operation == 1 ? 5
                            : f.anisotropy == 16                                      ? 6
                                                                                      : 3;

                        for (uint32_t variant = 0; variant < variants; variant++) {
                            SampleCase c = {};
                            c.textureIndex = textureSlot;
                            c.samplerIndex = 7 + sampler * 12;
                            c.operation = operation;
                            c.uv[0] = uv[0];
                            c.uv[1] = uv[1];
                            c.lod = lods[variant % 5];
                            const double footprint = std::exp2(double(lods[variant % 5]));
                            c.dx[0] = float(footprint / images[imageIndex].mips[view.base].width);
                            c.dy[1] = float(footprint / images[imageIndex].mips[view.base].height);
                            if (operation == 2 && variant >= 3) {
                                const float major = float(4u << (variant - 3));
                                const float w = float(images[imageIndex].mips[view.base].width), h = float(images[imageIndex].mips[view.base].height);
                                c.dx[0] = major / w;
                                c.dy[1] = 1 / h;

                                if (variant == 4) {
                                    c.dx[0] = major * .8f / w;
                                    c.dx[1] = major * .6f / h;
                                    c.dy[0] = -.6f / w;
                                    c.dy[1] = .8f / h;
                                }
                                c.reserved = constant ? 0 : 1;
                            }
                            cases.push_back(c);
                            const auto bounds = ValueBounds(images[imageIndex], view);
                            expected.push_back({c.reserved ? bounds[0] : Reference(images[imageIndex], view, sampler, c), c.reserved ? bounds[1] : Reference(images[imageIndex], view, sampler, c, true), images[imageIndex].mips[view.base].width, images[imageIndex].mips[view.base].height, view.count, sampler});
                        }
                    }
                }
                group.count = uint32_t(cases.size()) - group.first;
                groups.push_back(group);
            }
        }
        batch.recordingOwners.push_back(std::move(owner));
    }
    const uint64_t outputBytes = expected.size() * sizeof(SampleResult);
    Check(std::gcd(uint32_t(cases.size()), 97u) == 1 && std::gcd(uint32_t(cases.size()) / 2, 97u) == 1, "heap lane permutation must cover each output exactly once");
    static_assert(sizeof(Expected) == 80, "retained double-precision ideal reference and decode-bound artifact");
    Write(artifacts / (prefix + ".reference"), expected.data(), expected.size() * sizeof(Expected));
    batch.input = f.MakeBuffer(cases.size() * sizeof(SampleCase), sizeof(SampleCase), MemoryLocation::HOST_UPLOAD, BufferUsageBits::SHADER_RESOURCE);
    void* mapped = f.core.MapBuffer(*batch.input, 0, cases.size() * sizeof(SampleCase));
    Check(mapped != nullptr, "input map");
    std::memcpy(mapped, cases.data(), cases.size() * sizeof(SampleCase));
    f.core.UnmapBuffer(*batch.input);
    batch.inputView = f.MakeBufferView(batch.input, false);
    Write(artifacts / (prefix + ".cases"), cases.data(), cases.size() * sizeof(SampleCase));

    for (uint32_t i = 0; i < 2; i++) {
        batch.output[i] = f.MakeBuffer(outputBytes, sizeof(SampleResult), MemoryLocation::DEVICE, BufferUsageBits::SHADER_RESOURCE_STORAGE);
        batch.outputViews[i] = f.MakeBufferView(batch.output[i], true);
        batch.readback[i] = f.MakeBuffer(outputBytes, 0, MemoryLocation::HOST_READBACK, BufferUsageBits::NONE);
    }
    const WriteResourceDescriptorsDesc writes[] = {{batch.inputView, 199}, {batch.outputViews[0], 211}};
    Success(f.heapApi.WriteResourceDescriptors(*f.heap, writes, 2), "buffer heap writes");
    DescriptorPoolDesc poolDesc = {};
    poolDesc.descriptorSetMaxNum = uint32_t(groups.size());
    poolDesc.structuredBufferMaxNum = poolDesc.storageStructuredBufferMaxNum = poolDesc.textureMaxNum = poolDesc.samplerMaxNum = poolDesc.descriptorSetMaxNum;
    Success(f.core.CreateDescriptorPool(*f.device, poolDesc, batch.pool), "conventional pool");

    for (auto& group : groups) {
        Success(f.core.AllocateDescriptorSets(*batch.pool, *f.layouts[1], 0, &group.set, 1, 0), "conventional set");
        const UpdateDescriptorRangeDesc updates[] = {{group.set, 0, 0, &batch.inputView, 1}, {group.set, 1, 0, &batch.outputViews[1], 1}, {group.set, 2, 0, &group.texture, 1}, {group.set, 3, 0, &f.samplers[group.sampler], 1}};
        f.core.UpdateDescriptorRanges(updates, 4);
    }
    f.core.ResetCommandAllocator(*f.allocator);
    Success(f.core.BeginCommandBuffer(*f.command, nullptr), "begin");

    for (auto output : batch.output)
        f.BufferBarrier(output, AccessBits::NONE, StageBits::NONE, AccessBits::SHADER_RESOURCE_STORAGE, StageBits::COMPUTE_SHADER);
    // A single heap dispatch independently selects all texture and sampler slots
    f.heapApi.CmdSetDescriptorHeap(*f.command, *f.heap);
    f.core.CmdSetPipelineLayout(*f.command, BindPoint::COMPUTE, *f.layouts[0]);
    f.core.CmdSetPipeline(*f.command, *f.pipelines[0]);
    uint32_t root[] = {199, 211, 0, uint32_t(cases.size())};
    f.core.CmdSetRootConstants(*f.command, {0, root, sizeof(root), 0, BindPoint::COMPUTE});
    f.core.CmdDispatch(*f.command, {(uint32_t(cases.size()) + 63) / 64, 1, 1});
    f.core.CmdSetDescriptorPool(*f.command, *batch.pool);
    f.core.CmdSetPipelineLayout(*f.command, BindPoint::COMPUTE, *f.layouts[1]);
    f.core.CmdSetPipeline(*f.command, *f.pipelines[1]);

    for (const auto& group : groups) {
        f.core.CmdSetDescriptorSet(*f.command, {0, group.set});
        root[2] = group.first;
        root[3] = group.count;
        f.core.CmdSetRootConstants(*f.command, {0, root, sizeof(root), 0, BindPoint::COMPUTE});
        f.core.CmdDispatch(*f.command, {(group.count + 63) / 64, 1, 1});
    }
    // Explicitly restore the shared heap after conventional work and overwrite
    // half the direct result under an ordered UAV barrier
    f.BufferBarrier(batch.output[0], AccessBits::SHADER_RESOURCE_STORAGE, StageBits::COMPUTE_SHADER, AccessBits::SHADER_RESOURCE_STORAGE, StageBits::COMPUTE_SHADER);
    f.heapApi.CmdSetDescriptorHeap(*f.command, *f.heap);
    f.core.CmdSetPipelineLayout(*f.command, BindPoint::COMPUTE, *f.layouts[0]);
    f.core.CmdSetPipeline(*f.command, *f.pipelines[0]);
    root[2] = uint32_t(cases.size()) / 2;
    root[3] = uint32_t(cases.size()) - root[2];
    f.core.CmdSetRootConstants(*f.command, {0, root, sizeof(root), 0, BindPoint::COMPUTE});
    f.core.CmdDispatch(*f.command, {(root[3] + 63) / 64, 1, 1});

    for (uint32_t i = 0; i < 2; i++) {
        f.BufferBarrier(batch.output[i], AccessBits::SHADER_RESOURCE_STORAGE, StageBits::COMPUTE_SHADER, AccessBits::COPY_SOURCE, StageBits::COPY);
        f.core.CmdCopyBuffer(*f.command, *batch.readback[i], 0, *batch.output[i], 0, outputBytes);
    }
    Success(f.core.EndCommandBuffer(*f.command), "end");
    std::weak_ptr<TextureOwner> weak[] = {batch.recordingOwners[0], batch.recordingOwners[1]};
    Check(!weak[0].expired() && !weak[1].expired(), "recorded owners retained before submission");
    batch.retiredOwners = std::move(batch.recordingOwners);
    const uint32_t beforeDestroyed = f.destroyedOwners;
    const uint64_t retireAt = ++f.completion;
    FenceSubmitDesc signal = {f.fence, retireAt, StageBits::ALL};
    QueueSubmitDesc submit = {};
    submit.commandBuffers = &f.command;
    submit.commandBufferNum = 1;
    submit.signalFences = &signal;
    submit.signalFenceNum = 1;
    Success(f.core.QueueSubmit(*f.queue, submit), "submit");
    batch.submitted = true;
    Check(!weak[0].expired() && !weak[1].expired() && f.destroyedOwners == beforeDestroyed, "actual native owner lease survives submission");
    f.core.Wait(*f.fence, retireAt);
    Check(f.core.GetFenceValue(*f.fence) >= retireAt, "retirement fence");
    double maximumError = 0, parityError = 0;
    uint32_t qualityExceedances = 0;
    uint32_t scalarCases = 0, boundedCases = 0;
    const SampleResult* results[2] = {};

    for (uint32_t i = 0; i < 2; i++) {
        results[i] = (const SampleResult*)f.core.MapBuffer(*batch.readback[i], 0, outputBytes);
        Check(results[i] != nullptr, "readback map");
        Write(artifacts / (prefix + (i ? ".conventional" : ".heap")), results[i], size_t(outputBytes));
    }

    for (size_t i = 0; i < cases.size(); i++) {
        (cases[i].reserved ? boundedCases : scalarCases)++;
        for (uint32_t transport = 0; transport < 2; transport++) {
            const auto& value = results[transport][i];
            Check(value.width == expected[i].width && value.height == expected[i].height && value.levels == expected[i].levels && value.reserved == 0, "native partial-mip view dimensions");
            const double filterTolerance = (expected[i].sampler & 2) ? (2.0 / 255 + 1e-5) : (1.0 / 255 + 1e-5);

            for (uint32_t c = 0; c < 4; c++) {
                const double error = cases[i].reserved ? 0 : std::abs(value.color[c] - expected[i].value[c]);
                maximumError = std::max(maximumError, error);
                qualityExceedances += error > filterTolerance;
                const double tolerance = filterTolerance + expected[i].decodeBound[c];

                if (cases[i].reserved)
                    Check(value.color[c] >= (expected[i].value[c] - 1e-5) && value.color[c] <= (expected[i].decodeBound[c] + 1e-5), "anisotropic patterned result outside independent value bounds");

                if (!std::isfinite(value.color[c]) || error > tolerance) {
                    std::fprintf(stderr, "profile %s case %zu transport %u channel %u actual %.9g reference %.9g tolerance %.9g\n", prefix.c_str(), i, transport, c, value.color[c], expected[i].value[c], tolerance);
                    throw std::runtime_error("independent sampled reference mismatch");
                }
                const double parity = std::abs(double(results[0][i].color[c]) - results[1][i].color[c]);
                parityError = std::max(parityError, parity);
                Check(parity <= 1e-6, "heap versus conventional parity");
            }
        }
    }

    for (auto readback : batch.readback)
        f.core.UnmapBuffer(*readback);
    batch.retiredOwners.clear();
    Check(weak[0].expired() && weak[1].expired() && f.destroyedOwners == (beforeDestroyed + 2), "actual texture/view owners destroyed after completion");
    Check(errors == firstValidationErrors, "native/OpenRHI validation errors");
    std::ofstream receipt(artifacts / (prefix + ".json"));
    receipt << "{\"reference_policy\":2,\"format\":" << uint32_t(encoding) << ",\"original_width\":" << width << ",\"original_height\":" << height << ",\"epoch\":" << epoch << ",\"constant_field\":" << (constant ? "true" : "false") << ",\"cases\":" << cases.size() << ",\"scalar_reference_cases\":" << scalarCases << ",\"bounded_anisotropic_cases\":" << boundedCases << ",\"reference_max_error\":" << maximumError << ",\"strict_quality_channel_exceedances\":" << qualityExceedances << ",\"parity_max_error\":" << parityError << ",\"retired_native_owners\":2,\"completion\":" << retireAt << ",\"anisotropy\":" << f.anisotropy << ",\"passed\":true}\n";
    Check(receipt.good(), "profile receipt");
    std::printf("BC profile %s: %zu cases, max reference error %.8g, parity %.8g, two actual owners retired PASS\n", prefix.c_str(), cases.size(), maximumError, parityError);
}

int main(int argc, char** argv) {
    try {
        const uint32_t cpuChecks = SelfTest();

        if (argc == 2 && std::strcmp(argv[1], "--cpu-only") == 0) {
            std::printf("{\"status\":\"cpu_reference_passed\",\"checks\":%u,\"gpu_executed\":false}\n", cpuChecks);

            return 0;
        }
        Check(argc == 5 && (std::strcmp(argv[1], "--native") == 0 || std::strcmp(argv[1], "--native-aniso16") == 0), "usage: --cpu-only OR --native[-aniso16] heap.dxil conventional.dxil artifact-directory");
        const uint32_t anisotropy = std::strcmp(argv[1], "--native-aniso16") == 0 ? 16 : 1;
        const uint32_t profileCount = anisotropy == 16 ? 32 : 24, sampleCount = anisotropy == 16 ? 36864 : 41472;
        std::filesystem::path artifacts(argv[4]);

        if (std::filesystem::exists(artifacts))
            artifacts /= "run-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        Check(!std::filesystem::exists(artifacts), "native artifact directory must be fresh");
        std::filesystem::create_directories(artifacts);
        uint32_t failedProfiles = 0;
        {
            Fixture fixture;
            fixture.Create(argv[2], argv[3], anisotropy);
            const auto& desc = fixture.core.GetDeviceDesc(*fixture.device);
            std::ofstream identity(artifacts / "device.json");
            identity << "{\"adapter\":" << Quoted(desc.adapterDesc.name) << ",\"driver\":" << desc.adapterDesc.nativeDriverVersion << ",\"vendor\":" << desc.adapterDesc.vendorId << ",\"device\":" << desc.adapterDesc.deviceId << ",\"uid_low\":" << desc.adapterDesc.uid.low << ",\"uid_high\":" << desc.adapterDesc.uid.high;
#if defined(_WIN32)
            wchar_t path[32768] = {};
            Check(GetModuleFileNameW(GetModuleHandleW(L"OpenRHI.dll"), path, 32768) > 0, "loaded SDK module path");
            identity << ",\"library_path\":" << Quoted(std::filesystem::path(path).u8string());
#endif
            identity << ",\"api\":\"D3D12\",\"native_validation\":true,\"anisotropy\":" << anisotropy << ",\"sampler_factories\":[";

            for (uint32_t i = 0; i < 4; i++) {
                const auto& s = fixture.samplerDescriptions[i];
                identity << (i ? "," : "") << "{\"slot\":" << (7 + i * 12) << ",\"success\":true,\"filters\":[" << uint32_t(s.filters.min) << "," << uint32_t(s.filters.mag) << "," << uint32_t(s.filters.mip) << "],\"filter_op\":" << uint32_t(s.filters.op) << ",\"address\":[" << uint32_t(s.addressModes.u) << "," << uint32_t(s.addressModes.v) << "," << uint32_t(s.addressModes.w) << "],\"anisotropy\":" << uint32_t(s.anisotropy) << ",\"mip_bias\":" << s.mipBias << ",\"mip_min\":" << s.mipMin << ",\"mip_max\":" << s.mipMax << ",\"compare\":" << uint32_t(s.compareOp) << ",\"integer\":" << (s.isInteger ? "true" : "false") << ",\"unnormalized\":" << (s.unnormalizedCoordinates ? "true" : "false") << ",\"border_u32\":[" << s.borderColor.ui.x << "," << s.borderColor.ui.y << "," << s.borderColor.ui.z << "," << s.borderColor.ui.w << "]}";
            }
            identity << "]}\n";
            Check(identity.good(), "device receipt");

            const std::vector<std::array<uint32_t, 2>> dimensionsToTest = anisotropy == 16 ? std::vector<std::array<uint32_t, 2>>{{20, 12}, {4, 20}} : std::vector<std::array<uint32_t, 2>>{{20, 12}, {17, 9}, {4, 20}};

            for (Encoding format : {Encoding::BC4, Encoding::BC5, Encoding::BC7, Encoding::BC7Srgb})
                for (const auto dimensions : dimensionsToTest)
                    for (uint32_t epoch = 0; epoch < 2; epoch++) {
                        for (uint32_t family = 0; family < (anisotropy == 16 ? 2u : 1u); family++) {
                            try {
                                RunProfile(fixture, format, dimensions[0], dimensions[1], epoch, artifacts, family == 1);
                            } catch (const std::exception& failure) {
                                failedProfiles++;
                                const auto name = ProfileName(format, dimensions[0], dimensions[1], epoch, anisotropy, family == 1);
                                std::ofstream report(artifacts / (name + ".failed.json"));
                                report << "{\"passed\":false,\"error\":" << Quoted(failure.what()) << "}\n";
                                std::fprintf(stderr, "Profile %s failed: %s\n", name.c_str(), failure.what());
                            }
                        }
                    }
        }
        std::ofstream summary(artifacts / "summary.json");
        summary << "{\"reference_policy\":2,\"passed_profiles\":" << (profileCount - failedProfiles) << ",\"failed_profiles\":" << failedProfiles << ",\"validation_errors\":" << errors.load() << ",\"anisotropy\":" << anisotropy << "}\n";
        Check(summary.good(), "terminal summary");
        Check(failedProfiles == 0, "one or more native profiles failed; retained per-profile evidence");
        Check(errors == 0, "final validation errors");
        std::printf("{\"status\":\"native_passed\",\"profiles\":%u,\"cases\":%u,\"actual_native_owners_retired\":%u,\"validation_errors\":%u,\"anisotropy\":%u}\n", profileCount, sampleCount, profileCount * 2, errors.load(), anisotropy);

        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Bindless material qualification FAIL: %s\n", error.what());

        return 1;
    }
}
