// Native UI pixel qualification using the public facade and caller-owned resources
#include "OpenRHI.h"
#if OPENRHI_ENABLE_AGILITY_SDK_SUPPORT
#    include "OpenRHIAgilitySDK.h"
#endif
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIImgui.h"
#include "Extensions/OpenRHIRmlUi.h"
#include "Extensions/OpenRHIStreamer.h"

using namespace openrhi;
static uint32_t errors = 0;

static void Require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

#define REQUIRE(x) Require(bool(x), #x)

static void OPENRHI_CALL MessageCallback(Message type, const char*, uint32_t, const char* text, void*) {
    if (type == Message::ERROR)
        errors++;
    std::fprintf(stderr, "%s\n", text);
}

static void OPENRHI_CALL NoAbort(void*) {
}

struct Fixture {
    Device* device = nullptr;
    CoreInterface core = {};
    RmlUiInterface ui = {};
    ImguiInterface imgui = {};
    StreamerInterface streaming = {};
    Queue* queue = nullptr;
    CommandAllocator* allocator = nullptr;
    CommandBuffer* command = nullptr;
    Fence* fence = nullptr;
    uint64_t fenceValue = 0;
    std::vector<Buffer*> buffers;
    std::vector<Texture*> textures;
    std::vector<Descriptor*> descriptors;
    RmlUi* rmlUi = nullptr;
    Imgui* im = nullptr;
    Streamer* streamer = nullptr;
    Buffer* vertices = nullptr;
    Buffer* indices = nullptr;
    Buffer* pixels = nullptr;
    Texture* target = nullptr;
    Descriptor* attachment = nullptr;
    Descriptor* white = nullptr;
    Descriptor* tinted = nullptr;
    uint32_t rowPitch = 0;

    ~Fixture() {
        if (!device)
            return;
        if (im)
            imgui.DestroyImgui(im);
        if (rmlUi)
            ui.DestroyRmlUi(rmlUi);
        if (streamer)
            streaming.DestroyStreamer(streamer);
        for (auto descriptor : descriptors)
            core.DestroyDescriptor(descriptor);
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

    Buffer* MakeBuffer(uint64_t bytes, MemoryLocation location, BufferUsageBits usage = BufferUsageBits::NONE, const void* data = nullptr) {
        BufferDesc desc = {};
        desc.size = bytes;
        desc.usage = usage;
        Buffer* buffer = nullptr;
        REQUIRE(core.CreateCommittedBuffer(*device, location, 0, desc, buffer) == Result::SUCCESS);
        buffers.push_back(buffer);
        if (data) {
            void* mapped = core.MapBuffer(*buffer, 0, bytes);
            REQUIRE(mapped);
            memcpy(mapped, data, size_t(bytes));
            core.UnmapBuffer(*buffer);
        }
        return buffer;
    }

    Texture* MakeTexture(uint16_t width, uint16_t height, TextureUsageBits usage, Format format) {
        TextureDesc desc = {};
        desc.type = TextureType::TEXTURE_2D;
        desc.width = width;
        desc.height = height;
        desc.depth = desc.layerNum = desc.mipNum = desc.sampleNum = 1;
        desc.usage = usage;
        desc.format = format;
        Texture* texture = nullptr;
        REQUIRE(core.CreateCommittedTexture(*device, MemoryLocation::DEVICE, 0, desc, texture) == Result::SUCCESS);
        textures.push_back(texture);
        return texture;
    }

    Descriptor* View(Texture* texture, TextureView type, Format format) {
        TextureViewDesc desc = {};
        desc.texture = texture;
        desc.type = type;
        desc.format = format;
        Descriptor* view = nullptr;
        REQUIRE(core.CreateTextureView(desc, view) == Result::SUCCESS);
        descriptors.push_back(view);
        return view;
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
        REQUIRE(core.GetFenceValue(*fence) >= fenceValue);
    }

    void Transition(Texture* texture, AccessLayoutStage before, AccessLayoutStage after) {
        TextureBarrierDesc transition = {};
        transition.texture = texture;
        transition.before = before;
        transition.after = after;
        BarrierDesc barriers = {};
        barriers.textures = &transition;
        barriers.textureNum = 1;
        core.CmdBarrier(*command, barriers);
    }

    Descriptor* UploadTexture(uint8_t red) {
        Texture* texture = MakeTexture(1, 1, TextureUsageBits::SHADER_RESOURCE, Format::RGBA8_UNORM);
        std::vector<uint8_t> data(rowPitch, 0);
        data[0] = red;
        data[1] = data[2] = data[3] = 255;
        Buffer* upload = MakeBuffer(rowPitch, MemoryLocation::HOST_UPLOAD, BufferUsageBits::NONE, data.data());
        Transition(texture, {}, {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::COPY});
        TextureDataLayoutDesc layout = {0, rowPitch, rowPitch};
        TextureRegionDesc region = {};
        region.width = region.height = region.depth = 1;
        core.CmdUploadBufferToTexture(*command, *texture, region, *upload, layout);
        Transition(texture, {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::COPY}, {AccessBits::SHADER_RESOURCE, Layout::SHADER_RESOURCE, StageBits::FRAGMENT_SHADER});
        return View(texture, TextureView::TEXTURE, Format::RGBA8_UNORM);
    }

    void Rendering(AccessLayoutStage before, bool transparent = false) {
        Transition(target, before, {AccessBits::COLOR_ATTACHMENT, Layout::COLOR_ATTACHMENT, StageBits::COLOR_ATTACHMENT});
        AttachmentDesc color = {};
        color.descriptor = attachment;
        color.loadOp = LoadOp::CLEAR;
        color.storeOp = StoreOp::STORE;
        color.clearValue.color.f = transparent ? Color32f{0, 0, 0, 0} : Color32f{0, 0, 1, 1};
        RenderingDesc rendering = {};
        rendering.colors = &color;
        rendering.colorNum = 1;
        core.CmdBeginRendering(*command, rendering);
    }

    void Readback() {
        core.CmdEndRendering(*command);
        Transition(target, {AccessBits::COLOR_ATTACHMENT, Layout::COLOR_ATTACHMENT, StageBits::COLOR_ATTACHMENT}, {AccessBits::COPY_SOURCE, Layout::COPY_SOURCE, StageBits::COPY});
        TextureDataLayoutDesc layout = {0, rowPitch, rowPitch * 64};
        TextureRegionDesc region = {};
        region.width = region.height = 64;
        region.depth = 1;
        core.CmdReadbackTextureToBuffer(*command, *pixels, layout, *target, region);
        Submit();
    }
};

int main(int argc, char** argv) {
    try {
        Fixture f;
        DeviceCreationDesc creation = {};
        creation.graphicsAPI = (argc > 1 && !strcmp(argv[1], "VK")) ? GraphicsAPI::VK : ((argc > 1 && !strcmp(argv[1], "METAL")) ? GraphicsAPI::METAL : GraphicsAPI::D3D12);
        creation.enableOpenRHIValidation = true;
        creation.enableGraphicsAPIValidation = true;
        creation.callbackInterface = {MessageCallback, NoAbort, nullptr};
        Result created = openrhiCreateDevice(creation, f.device);
        if (created == Result::UNSUPPORTED)
            return 77;
        REQUIRE(created == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*f.device, OPENRHI_INTERFACE(CoreInterface), &f.core) == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*f.device, OPENRHI_INTERFACE(RmlUiInterface), &f.ui) == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*f.device, OPENRHI_INTERFACE(ImguiInterface), &f.imgui) == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*f.device, OPENRHI_INTERFACE(StreamerInterface), &f.streaming) == Result::SUCCESS);
        auto caps = f.ui.GetRmlUiCapabilities(*f.device);
        REQUIRE(caps.nativeRendering && caps.geometry && caps.textures && caps.scissor && caps.transforms && !caps.clipMasks && !caps.layers && !caps.filters && !caps.customShaders);
        REQUIRE(f.core.GetQueue(*f.device, QueueType::GRAPHICS, 0, f.queue) == Result::SUCCESS);
        REQUIRE(f.core.CreateCommandAllocator(*f.queue, f.allocator) == Result::SUCCESS);
        REQUIRE(f.core.CreateCommandBuffer(*f.allocator, f.command) == Result::SUCCESS);
        REQUIRE(f.core.CreateFence(*f.device, 0, f.fence) == Result::SUCCESS);
        f.rowPitch = std::max(256u, f.core.GetDeviceDesc(*f.device).memoryAlignment.uploadBufferTextureRow);
        f.pixels = f.MakeBuffer(uint64_t(f.rowPitch) * 64, MemoryLocation::HOST_READBACK);
        RmlUiVertex vertices[] = {{{0, 0}, {128, 0, 0, 128}, {0, 0}}, {{16, 0}, {128, 0, 0, 128}, {1, 0}}, {{16, 16}, {128, 0, 0, 128}, {1, 1}}, {{0, 16}, {128, 0, 0, 128}, {0, 1}}};
        uint32_t indices[] = {0, 1, 2, 0, 2, 3};
        f.vertices = f.MakeBuffer(sizeof(vertices), MemoryLocation::HOST_UPLOAD, BufferUsageBits::VERTEX, vertices);
        f.indices = f.MakeBuffer(sizeof(indices), MemoryLocation::HOST_UPLOAD, BufferUsageBits::INDEX, indices);
        f.Begin();
        f.white = f.UploadTexture(255);
        f.tinted = f.UploadTexture(128);
        f.Submit();
        RmlUiDesc desc = {Format::RGBA8_UNORM, 2};
        RmlUi* rejected = reinterpret_cast<RmlUi*>(uintptr_t(1));
        REQUIRE(f.ui.CreateRmlUi(*f.device, {Format::RGBA8_UNORM, 0}, rejected) == Result::INVALID_ARGUMENT && !rejected);
        REQUIRE(errors == 1);
        errors = 0;
        REQUIRE(f.ui.CreateRmlUi(*f.device, desc, f.rmlUi) == Result::SUCCESS);
        f.target = f.MakeTexture(64, 64, TextureUsageBits::COLOR_ATTACHMENT, desc.attachmentFormat);
        f.attachment = f.View(f.target, TextureView::COLOR_ATTACHMENT, desc.attachmentFormat);
        DrawRmlUiDesc draws[2] = {};
        for (auto& draw : draws) {
            draw.geometry = {{f.vertices, 0}, {f.indices, 0}, 4, 6};
            draw.transform[0] = draw.transform[5] = draw.transform[10] = draw.transform[15] = 1;
            draw.texture = f.white;
            draw.scissor = {0, 0, 64, 64};
        }
        draws[0].texture = f.tinted;
        draws[0].translation[0] = 4;
        draws[0].translation[1] = 2;
        draws[0].transform[12] = 4;
        draws[0].transform[13] = 6;
        draws[0].scissor = {12, 12, 8, 8};
        draws[1].translation[0] = draws[1].translation[1] = 32;
        for (uint32_t frame = 0; frame < 2; frame++) {
            f.ui.ResetRmlUi(*f.rmlUi);
            f.Begin();
            f.Rendering(frame ? AccessLayoutStage{AccessBits::COPY_SOURCE, Layout::COPY_SOURCE, StageBits::COPY} : AccessLayoutStage{});
            if (!frame) {
                auto invalid = draws[0];
                invalid.geometry.indices.offset = 1;
                REQUIRE(f.ui.CmdDrawRmlUi(*f.command, *f.rmlUi, {64, 64}, &invalid, 1) == Result::INVALID_ARGUMENT);
                invalid = draws[0];
                invalid.geometry.vertexNum = UINT32_MAX;
                REQUIRE(f.ui.CmdDrawRmlUi(*f.command, *f.rmlUi, {64, 64}, &invalid, 1) == Result::INVALID_ARGUMENT);
                invalid = draws[0];
                invalid.transform[0] = std::numeric_limits<float>::quiet_NaN();
                REQUIRE(f.ui.CmdDrawRmlUi(*f.command, *f.rmlUi, {64, 64}, &invalid, 1) == Result::INVALID_ARGUMENT);
                invalid = draws[0];
                invalid.scissor.x = -1;
                REQUIRE(f.ui.CmdDrawRmlUi(*f.command, *f.rmlUi, {64, 64}, &invalid, 1) == Result::INVALID_ARGUMENT);
                REQUIRE(f.ui.CmdDrawRmlUi(*f.command, *f.rmlUi, {64, 64}, nullptr, 1) == Result::INVALID_ARGUMENT);
                REQUIRE(errors == 5);
                errors = 0;
            }
            REQUIRE(f.ui.CmdDrawRmlUi(*f.command, *f.rmlUi, {64, 64}, draws, 2) == Result::SUCCESS);
            REQUIRE(f.ui.CmdDrawRmlUi(*f.command, *f.rmlUi, {64, 64}, draws, 1) == Result::OUT_OF_MEMORY);
            f.Readback();
            auto* result = (const uint8_t*)f.core.MapBuffer(*f.pixels, 0, uint64_t(f.rowPitch) * 64);
            REQUIRE(result);
            for (uint32_t y = 0; y < 64; y++)
                for (uint32_t x = 0; x < 64; x++) {
                    const bool first = x >= 12 && x < 20 && y >= 12 && y < 20;
                    const bool second = x >= 32 && x < 48 && y >= 32 && y < 48;
                    const auto* p = result + y * f.rowPitch + x * 4;
                    REQUIRE(std::abs(int(p[0]) - (first ? 64 : (second ? 128 : 0))) <= 1);
                    REQUIRE(p[1] == 0 && std::abs(int(p[2]) - ((first || second) ? 127 : 255)) <= 1 && p[3] == 255);
                }
            f.core.UnmapBuffer(*f.pixels);
        }

        // Verify premultiplied sRGB UI conversion on an sRGB attachment
        f.ui.DestroyRmlUi(f.rmlUi);
        f.rmlUi = nullptr;
        desc.attachmentFormat = Format::RGBA8_SRGB;
        desc.linearColor = true;
        REQUIRE(f.ui.CreateRmlUi(*f.device, desc, f.rmlUi) == Result::SUCCESS);
        f.target = f.MakeTexture(64, 64, TextureUsageBits::COLOR_ATTACHMENT, desc.attachmentFormat);
        f.attachment = f.View(f.target, TextureView::COLOR_ATTACHMENT, desc.attachmentFormat);
        f.Begin();
        f.Rendering({});
        REQUIRE(f.ui.CmdDrawRmlUi(*f.command, *f.rmlUi, {64, 64}, draws, 2) == Result::SUCCESS);
        f.Readback();
        auto* linearResult = (const uint8_t*)f.core.MapBuffer(*f.pixels, 0, uint64_t(f.rowPitch) * 64);
        REQUIRE(linearResult);
        const auto* firstLinear = linearResult + f.rowPitch * 15 + 15 * 4;
        const auto* secondLinear = linearResult + f.rowPitch * 40 + 40 * 4;
        REQUIRE(std::abs(int(firstLinear[0]) - 93) <= 2 && std::abs(int(firstLinear[2]) - 187) <= 2 && firstLinear[3] == 255);
        REQUIRE(std::abs(int(secondLinear[0]) - 188) <= 2 && std::abs(int(secondLinear[2]) - 187) <= 2 && secondLinear[3] == 255);
        f.core.UnmapBuffer(*f.pixels);

        // Exercise the actual pinned ImGui texture ABI, font atlas upload, streaming and draw recording
        IMGUI_CHECKVERSION();
        static_assert(sizeof(ImDrawVert) == 20 && sizeof(ImDrawIdx) == 2 && sizeof(ImTextureData) == 96, "Pinned ImGui ABI");
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        io.DisplaySize = {64, 64};
        io.DeltaTime = 1.0f / 60;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        io.IniFilename = nullptr;
        StreamerDesc stream = {};
        stream.constantBufferMemoryLocation = MemoryLocation::HOST_UPLOAD;
        stream.dynamicBufferMemoryLocation = MemoryLocation::HOST_UPLOAD;
        stream.dynamicBufferDesc.usage = BufferUsageBits::VERTEX | BufferUsageBits::INDEX;
        stream.queuedFrameNum = 1;
        stream.hostDataCapacity = 1024 * 1024;
        REQUIRE(f.streaming.CreateStreamer(*f.device, stream, f.streamer) == Result::SUCCESS);
        REQUIRE(f.imgui.CreateImgui(*f.device, {8}, f.im) == Result::SUCCESS);
        ImGui::NewFrame();
        ImGui::GetBackgroundDrawList()->AddRectFilled({4, 4}, {20, 20}, IM_COL32(255, 0, 255, 255));
        ImGui::Render();
        auto* data = ImGui::GetDrawData();
        CopyImguiDataDesc copy = {data->CmdLists.Data, uint32_t(data->CmdLists.Size), data->Textures->Data, uint32_t(data->Textures->Size)};
        ImguiRenderData render = {};
        f.Begin();
        f.imgui.CmdCopyImguiData(*f.command, *f.streamer, *f.im, copy, render);
        f.Rendering({AccessBits::COPY_SOURCE, Layout::COPY_SOURCE, StageBits::COPY});
        DrawImguiDesc draw = {{64, 64}, 1, Format::RGBA8_SRGB, true};
        f.imgui.CmdDrawImgui(*f.command, render, draw);
        f.Readback();
        auto* result = (const uint8_t*)f.core.MapBuffer(*f.pixels, 0, uint64_t(f.rowPitch) * 64);
        REQUIRE(result);
        const auto* inside = result + f.rowPitch * 10 + 10 * 4;
        REQUIRE(inside[0] == 255 && inside[1] == 0 && inside[2] == 255 && inside[3] == 255);
        const auto* outside = result + f.rowPitch * 30 + 30 * 4;
        REQUIRE(outside[0] == 0 && outside[1] == 0 && outside[2] == 255);
        f.core.UnmapBuffer(*f.pixels);
        ImGui::NewFrame();
        ImGui::GetBackgroundDrawList()->AddRectFilled({4, 4}, {20, 20}, IM_COL32(255, 0, 255, 128));
        ImGui::Render();
        data = ImGui::GetDrawData();
        copy = {data->CmdLists.Data, uint32_t(data->CmdLists.Size), data->Textures->Data, uint32_t(data->Textures->Size)};
        f.target = f.MakeTexture(64, 64, TextureUsageBits::COLOR_ATTACHMENT, Format::RGBA8_UNORM);
        f.attachment = f.View(f.target, TextureView::COLOR_ATTACHMENT, Format::RGBA8_UNORM);
        f.Begin();
        f.imgui.CmdCopyImguiData(*f.command, *f.streamer, *f.im, copy, render);
        f.Rendering({}, true);
        draw.attachmentFormat = Format::RGBA8_UNORM;
        draw.linearColor = false;
        draw.writeAlpha = true;
        f.imgui.CmdDrawImgui(*f.command, render, draw);
        f.Readback();
        result = (const uint8_t*)f.core.MapBuffer(*f.pixels, 0, uint64_t(f.rowPitch) * 64);
        REQUIRE(result);
        inside = result + f.rowPitch * 10 + 10 * 4;
        outside = result + f.rowPitch * 30 + 30 * 4;
        REQUIRE(inside[0] == 128 && inside[1] == 0 && inside[2] == 128 && inside[3] == 128);
        REQUIRE(outside[0] == 0 && outside[1] == 0 && outside[2] == 0 && outside[3] == 0);
        f.core.UnmapBuffer(*f.pixels);
        f.streaming.EndStreamerFrame(*f.streamer);
        ImGui::DestroyContext();
        REQUIRE(errors == 0);
        std::printf("UI native readback PASS: %s; RmlUi textured geometry/premultiplied blend/scissor/transform/descriptor budget+reset/sRGB/invalid inputs; ImGui 1.92.9 atlas/stream/draw; validation_errors=%u\n", f.core.GetDeviceDesc(*f.device).adapterDesc.name, errors);

        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "UI qualification failed: %s\n", error.what());

        return 1;
    }
}
