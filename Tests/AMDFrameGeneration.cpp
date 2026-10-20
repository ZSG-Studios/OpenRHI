// Native FSR SR, frame generation, presentation and Anti-Lag qualification through public OpenRHI interfaces
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "OpenRHI.h"
#include "OpenRHIAgilitySDK.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIFrameGeneration.h"
#include "Extensions/OpenRHIVendorFeatures.h"

using namespace openrhi;
#define REQUIRE(x) \
    do { \
        if (!(x)) \
            throw std::runtime_error(#x); \
    } while (false)
static uint32_t errors = 0;
static uint32_t expectedErrors = 0;
static bool expectError = false;

static void OPENRHI_CALL MessageCallback(Message type, const char*, uint32_t, const char* message, void*) {
    if (type == Message::ERROR) {
        if (expectError)
            expectedErrors++;
        else
            errors++;
    }
    std::fprintf(stderr, "%s\n", message);
}

static void OPENRHI_CALL NoAbort(void*) {
}

constexpr Dim_t width = 640, height = 360;

struct Image {
    Texture* texture = nullptr;
    Descriptor* srv = nullptr;
    Descriptor* uav = nullptr;
    Buffer* upload = nullptr;
    uint32_t rowPitch = 0;
    uint32_t pixelBytes = 0;
    std::vector<uint8_t> data;
};

struct Fixture {
    Device* device = nullptr;
    CoreInterface core = {};
    UpscalerInterface upscale = {};
    FrameGenerationInterface generation = {};
    AntiLagInterface anti = {};
    Queue* queue = nullptr;
    CommandAllocator* allocator = nullptr;
    CommandBuffer* command = nullptr;
    Fence* fence = nullptr;
    Upscaler* sr = nullptr;
    FrameGeneration* fg = nullptr;
    AntiLagContext* latency = nullptr;
    Buffer* readback = nullptr;
    HWND window = nullptr;
    uint64_t fenceValue = 0;
    Image color, depth, motion, output, ui;
    std::vector<Texture*> proxyTextures;

    ~Fixture() {
        if (!device)
            return;
        core.DeviceWaitIdle(device);
        if (fg) {
            FrameGenerationConfigureDesc disabled = {};
            generation.ConfigureFrameGeneration(*fg, disabled);
            generation.WaitFrameGeneration(*fg);
        }
        if (latency) {
            anti.AttachAntiLagSwapChain(*latency, nullptr);
            anti.DestroyAntiLagContext(latency);
        }
        if (fg)
            generation.DestroyFrameGeneration(fg);
        if (sr)
            upscale.DestroyUpscaler(sr);
        for (Image* image : {&color, &depth, &motion, &output, &ui}) {
            if (image->srv)
                core.DestroyDescriptor(image->srv);
            if (image->uav)
                core.DestroyDescriptor(image->uav);
            if (image->texture)
                core.DestroyTexture(image->texture);
            if (image->upload)
                core.DestroyBuffer(image->upload);
        }
        if (readback)
            core.DestroyBuffer(readback);
        if (fence)
            core.DestroyFence(fence);
        if (command)
            core.DestroyCommandBuffer(command);
        if (allocator)
            core.DestroyCommandAllocator(allocator);
        openrhiDestroyDevice(device);
        if (window)
            DestroyWindow(window);
    }

    Buffer* BufferAt(uint64_t bytes, MemoryLocation location) {
        BufferDesc desc = {};
        desc.size = bytes;
        Buffer* buffer = nullptr;
        REQUIRE(core.CreateCommittedBuffer(*device, location, 0, desc, buffer) == Result::SUCCESS);
        return buffer;
    }

    void CreateImage(Image& image, Format format, uint32_t bytes, bool storage = false) {
        TextureDesc desc = {};
        desc.type = TextureType::TEXTURE_2D;
        desc.format = format;
        desc.width = width;
        desc.height = height;
        desc.depth = desc.layerNum = desc.mipNum = desc.sampleNum = 1;
        desc.usage = TextureUsageBits::SHADER_RESOURCE | (storage ? TextureUsageBits::SHADER_RESOURCE_STORAGE : TextureUsageBits::NONE);
        REQUIRE(core.CreateCommittedTexture(*device, MemoryLocation::DEVICE, 0, desc, image.texture) == Result::SUCCESS);
        TextureViewDesc view = {};
        view.texture = image.texture;
        view.format = format;
        view.type = TextureView::TEXTURE;
        REQUIRE(core.CreateTextureView(view, image.srv) == Result::SUCCESS);
        if (storage) {
            view.type = TextureView::STORAGE_TEXTURE;
            REQUIRE(core.CreateTextureView(view, image.uav) == Result::SUCCESS);
        }
        const uint32_t alignment = core.GetDeviceDesc(*device).memoryAlignment.uploadBufferTextureRow;
        image.rowPitch = ((width * bytes + alignment - 1) / alignment) * alignment;
        image.pixelBytes = bytes;
        image.data.resize(size_t(image.rowPitch) * height);
        image.upload = BufferAt(image.data.size(), MemoryLocation::HOST_UPLOAD);
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
        REQUIRE(anti.MarkAntiLagEndOfRendering(*latency) == Result::SUCCESS);
        core.Wait(*fence, fenceValue);
        REQUIRE(core.GetFenceValue(*fence) >= fenceValue);
    }

    void Upload(Image& image, bool initialized) {
        void* mapped = core.MapBuffer(*image.upload, 0, image.data.size());
        REQUIRE(mapped);
        memcpy(mapped, image.data.data(), image.data.size());
        core.UnmapBuffer(*image.upload);
        const AccessLayoutStage read = {AccessBits::COMMON, Layout::GENERAL, StageBits::ALL};
        Transition(image.texture, initialized ? read : AccessLayoutStage{}, {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::COPY});
        TextureDataLayoutDesc layout = {0, image.rowPitch, image.rowPitch * height};
        TextureRegionDesc region = {};
        region.width = width;
        region.height = height;
        region.depth = 1;
        core.CmdUploadBufferToTexture(*command, *image.texture, region, *image.upload, layout);
        Transition(image.texture, {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::COPY}, read);
    }

    void Initialize(bool validation) {
        DeviceCreationDesc desc = {};
        desc.graphicsAPI = GraphicsAPI::D3D12;
        desc.enableOpenRHIValidation = validation;
        desc.enableGraphicsAPIValidation = true;
        desc.callbackInterface = {MessageCallback, NoAbort, nullptr};
        REQUIRE(openrhiCreateDevice(desc, device) == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(CoreInterface), &core) == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(UpscalerInterface), &upscale) == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(FrameGenerationInterface), &generation) == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(AntiLagInterface), &anti) == Result::SUCCESS);
        REQUIRE(core.GetQueue(*device, QueueType::GRAPHICS, 0, queue) == Result::SUCCESS);
        REQUIRE(core.CreateCommandAllocator(*queue, allocator) == Result::SUCCESS);
        REQUIRE(core.CreateCommandBuffer(*allocator, command) == Result::SUCCESS);
        REQUIRE(core.CreateFence(*device, 0, fence) == Result::SUCCESS);
        REQUIRE(upscale.IsUpscalerSupported(*device, UpscalerType::FSR));
        UpscalerDesc upscaleDesc = {};
        upscaleDesc.type = UpscalerType::FSR;
        upscaleDesc.mode = UpscalerMode::NATIVE;
        upscaleDesc.flags = UpscalerBits::SRGB;
        upscaleDesc.upscaleResolution = {width, height};
        REQUIRE(upscale.CreateUpscaler(*device, upscaleDesc, sr) == Result::SUCCESS);
        REQUIRE(anti.CreateAntiLagContext(*device, latency) == Result::SUCCESS);
        uint32_t count = 0;
        REQUIRE(generation.EnumerateFrameGenerationProviders(*device, nullptr, count) == Result::SUCCESS && count);
        std::vector<FrameGenerationProvider> providers(count);
        REQUIRE(generation.EnumerateFrameGenerationProviders(*device, providers.data(), count) == Result::SUCCESS);
        for (const auto& provider : providers)
            std::printf("FG eligible provider: %llu %s\n", (unsigned long long)provider.id, provider.name);
        window = CreateWindowExW(WS_EX_NOACTIVATE, L"STATIC", L"OpenRHI AMD native qualification", WS_OVERLAPPEDWINDOW, 32, 32, width, height, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        REQUIRE(window);
        ShowWindow(window, SW_SHOWNOACTIVATE);
        FrameGenerationDesc fgDesc = {};
        fgDesc.swapChain.window.windows.hwnd = window;
        fgDesc.swapChain.queue = queue;
        fgDesc.swapChain.width = width;
        fgDesc.swapChain.height = height;
        fgDesc.swapChain.textureNum = 2;
        fgDesc.swapChain.flags = validation ? (SwapChainBits::WAITABLE | SwapChainBits::ALLOW_TEARING) : SwapChainBits::NONE;
        fgDesc.swapChain.queuedFrameNum = 1;
        fgDesc.swapChain.format = SwapChainFormat::BT709_G22_8BIT;
        fgDesc.maxRenderResolution = {width, height};
        fgDesc.transferFunction = FrameGenerationTransferFunction::SRGB;
        fgDesc.enableDebugChecking = true;
        const Result created = generation.CreateFrameGeneration(*device, fgDesc, fg);
        std::printf("FG context creation=%d validation=%d\n", int(created), validation);
        REQUIRE(created == Result::SUCCESS);
        core.SetDebugName(fg, "AMD frame generation test");
        Texture* const* images = generation.GetFrameGenerationTextures(*fg, count);
        REQUIRE(images && count >= 2);
        proxyTextures.assign(images, images + count);
        REQUIRE(anti.AttachAntiLagSwapChain(*latency, generation.GetFrameGenerationSwapChainNativeObject(*fg)) == Result::SUCCESS);
        CreateImage(color, Format::RGBA8_UNORM, 4);
        CreateImage(depth, Format::R32_SFLOAT, 4);
        CreateImage(motion, Format::RG32_SFLOAT, 8);
        CreateImage(output, Format::RGBA8_UNORM, 4, true);
        CreateImage(ui, Format::RGBA8_UNORM, 4);
        readback = BufferAt(output.data.size(), MemoryLocation::HOST_READBACK);
    }

    void ValidateContracts() {
        const uint32_t before = expectedErrors;
        expectError = true;
        FrameGenerationDesc invalid = {};
        FrameGeneration* rejected = fg;
        REQUIRE(generation.CreateFrameGeneration(*device, invalid, rejected) == Result::INVALID_ARGUMENT && !rejected);
        FrameGenerationConfigureDesc configure = {};
        configure.enabled = true;
        REQUIRE(generation.ConfigureFrameGeneration(*fg, configure) == Result::INVALID_ARGUMENT);
        REQUIRE(generation.PresentFrameGeneration(*fg, 2) == Result::INVALID_ARGUMENT);
        Begin();
        FrameGenerationPrepareDesc prepare = {};
        REQUIRE(generation.CmdPrepareFrameGeneration(*command, *fg, prepare) == Result::INVALID_ARGUMENT);
        TextureBarrierDesc barrier = {};
        barrier.texture = color.texture;
        barrier.after = {AccessBits::COMMON, Layout::GENERAL, StageBits::NONE};
        BarrierDesc barriers = {};
        barriers.textures = &barrier;
        barriers.textureNum = 1;
        core.CmdBarrier(*command, barriers);
        barrier.after = {AccessBits::COMMON | AccessBits::COPY_SOURCE, Layout::GENERAL, StageBits::ALL};
        core.CmdBarrier(*command, barriers);
        barrier.after = {AccessBits::COMMON, Layout::SHADER_RESOURCE, StageBits::ALL};
        core.CmdBarrier(*command, barriers);
        Submit();
        expectError = false;
        REQUIRE(expectedErrors == before + 7);
    }

    void Frame(uint64_t frameId, bool enabled) {
        REQUIRE(anti.UpdateAntiLag(*latency, true, 0) == Result::SUCCESS);
        MSG message;
        while (PeekMessageW(&message, window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        uint32_t index = 0;
        REQUIRE(generation.AcquireFrameGenerationTexture(*fg, index) == Result::SUCCESS && index < proxyTextures.size());
        for (uint32_t y = 0; y < height; y++)
            for (uint32_t x = 0; x < width; x++) {
                const bool object = x >= 80 + frameId * 2 && x < 220 + frameId * 2 && y >= 80 && y < 220;
                auto* c = color.data.data() + y * color.rowPitch + x * 4;
                c[0] = object ? 220 : 32;
                c[1] = object ? 64 : uint8_t(32 + (x / 32) % 2 * 48);
                c[2] = object ? 32 : 128;
                c[3] = 255;
                const float d = object ? 0.5f : 1.0f;
                memcpy(depth.data.data() + y * depth.rowPitch + x * 4, &d, 4);
                const float mv[] = {object ? -2.0f : 0.0f, 0};
                memcpy(motion.data.data() + y * motion.rowPitch + x * 8, mv, 8);
                auto* u = ui.data.data() + y * ui.rowPitch + x * 4;
                const bool panel = x < 64 && y < 32;
                u[0] = u[2] = 0;
                u[1] = u[3] = panel ? 128 : 0;
            }
        FrameGenerationConfigureDesc configure = {};
        configure.frameId = frameId;
        configure.enabled = enabled;
        configure.hudlessColor = output.texture;
        configure.ui = ui.texture;
        configure.uiPremultiplied = true;
        REQUIRE(generation.ConfigureFrameGeneration(*fg, configure) == Result::SUCCESS);
        Begin();
        Upload(color, frameId > 1);
        Upload(depth, frameId > 1);
        Upload(motion, frameId > 1);
        Upload(ui, frameId > 1);
        const AccessLayoutStage read = {AccessBits::COMMON, Layout::GENERAL, StageBits::ALL};
        const AccessLayoutStage storage = {AccessBits::COMMON, Layout::GENERAL, StageBits::ALL};
        Transition(output.texture, frameId > 1 ? read : AccessLayoutStage{}, storage);
        DispatchUpscaleDesc dispatch = {};
        dispatch.input = {color.texture, color.srv};
        dispatch.output = {output.texture, output.uav};
        dispatch.guides.upscaler.depth = {depth.texture, depth.srv};
        dispatch.guides.upscaler.mv = {motion.texture, motion.srv};
        dispatch.currentResolution = {width, height};
        dispatch.mvScale = {1, 1};
        dispatch.settings.fsr = {0.1f, 1000, 1.0f, 16.667f, 1.0f, 0.2f};
        dispatch.flags = frameId == 1 ? DispatchUpscaleBits::RESET_HISTORY : DispatchUpscaleBits::NONE;
        upscale.CmdDispatchUpscale(*command, *sr, dispatch);
        Transition(output.texture, storage, read);
        if (enabled) {
            FrameGenerationPrepareDesc prepare = {};
            prepare.frameId = frameId;
            prepare.renderResolution = frameId >= 12 ? Dim2_t{width / 2, height / 2} : Dim2_t{width, height};
            prepare.depth = depth.texture;
            prepare.motionVectors = motion.texture;
            prepare.motionVectorScale = {1, 1};
            prepare.frameTimeMs = 16.667f;
            prepare.cameraNear = 0.1f;
            prepare.cameraFar = 1000;
            prepare.verticalFov = 1;
            prepare.viewSpaceToMetersFactor = 1;
            prepare.cameraUp[1] = prepare.cameraRight[0] = prepare.cameraForward[2] = 1;
            prepare.reset = frameId == 1;
            REQUIRE(generation.CmdPrepareFrameGeneration(*command, *fg, prepare) == Result::SUCCESS);
        }
        const AccessLayoutStage copySource = {AccessBits::COPY_SOURCE, Layout::COPY_SOURCE, StageBits::COPY};
        const AccessLayoutStage copyDest = {AccessBits::COPY_DESTINATION, Layout::COPY_DESTINATION, StageBits::COPY};
        const AccessLayoutStage present = {AccessBits::NONE, Layout::PRESENT, StageBits::NONE};
        Transition(output.texture, read, copySource);
        Transition(proxyTextures[index], present, copyDest);
        core.CmdCopyTexture(*command, *proxyTextures[index], nullptr, *output.texture, nullptr);
        TextureDataLayoutDesc layout = {0, output.rowPitch, output.rowPitch * height};
        TextureRegionDesc region = {};
        region.width = width;
        region.height = height;
        region.depth = 1;
        core.CmdReadbackTextureToBuffer(*command, *readback, layout, *output.texture, region);
        Transition(proxyTextures[index], copyDest, present);
        Transition(output.texture, copySource, read);
        Submit();
        REQUIRE(anti.SetAntiLagFrameType(*latency, false) == Result::SUCCESS);
        const Result presented = generation.PresentFrameGeneration(*fg, frameId >= 10 ? 1 : 0);
        FrameGenerationStats observed = {};
        generation.GetFrameGenerationStats(*fg, observed);
        std::printf("FG frame=%llu present=%d nativePresent=%d dispatch=%d prepared=%llu last=%llu\n", (unsigned long long)frameId, int(presented), int(observed.lastPresentResult), int(observed.lastDispatchResult), (unsigned long long)observed.preparedFrames, (unsigned long long)observed.lastFrameId);
        REQUIRE(presented == Result::SUCCESS);
        REQUIRE(generation.WaitFrameGeneration(*fg) == Result::SUCCESS);
        const auto* pixels = (const uint8_t*)core.MapBuffer(*readback, 0, output.data.size());
        REQUIRE(pixels && pixels[2] > pixels[0]);
        core.UnmapBuffer(*readback);
    }
};

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    try {
        for (bool validation : {false, true}) {
            Fixture f;
            f.Initialize(validation);

            if (validation)
                f.ValidateContracts();
            for (uint64_t frame = 1; frame <= 8; frame++)
                f.Frame(frame, true);
            FrameGenerationStats enabled = {};
            f.generation.GetFrameGenerationStats(*f.fg, enabled);
            std::printf("FG native provider=%s id=%llu prepare=%llu dispatch=%llu generated=%llu present=%llu dxgi=%u validation=%d errors=%u\n", enabled.provider.name, (unsigned long long)enabled.provider.id, (unsigned long long)enabled.preparedFrames, (unsigned long long)enabled.generatedDispatches, (unsigned long long)enabled.generatedFramesRecorded, (unsigned long long)enabled.presentCalls, enabled.dxgiPresentCount, validation, errors);
            REQUIRE(enabled.preparedFrames == 8 && enabled.generatedDispatches && enabled.generatedFramesRecorded && enabled.presentCalls == 8 && enabled.dxgiPresentCount > enabled.presentCalls);
            REQUIRE(f.generation.PresentFrameGeneration(*f.fg, 0) == Result::INVALID_ARGUMENT);
            f.Frame(9, false);
            FrameGenerationStats disabled = {};
            f.generation.GetFrameGenerationStats(*f.fg, disabled);
            REQUIRE(!disabled.configuredEnabled && disabled.generatedDispatches == enabled.generatedDispatches && disabled.presentCalls == 9);
            f.Frame(10, true);
            f.Frame(11, true);
            f.Frame(12, true); // active prepare region is smaller than the persistent guide allocation
            FrameGenerationStats resumed = {};
            f.generation.GetFrameGenerationStats(*f.fg, resumed);
            REQUIRE(resumed.configuredEnabled && resumed.preparedFrames == 11 && resumed.generatedDispatches > enabled.generatedDispatches && resumed.presentCalls == 12);
            std::printf("FG resume/VSync provider=%s dispatch=%llu generated=%llu present=%llu dxgi=%u validation=%d errors=%u\n", resumed.provider.name, (unsigned long long)resumed.generatedDispatches, (unsigned long long)resumed.generatedFramesRecorded, (unsigned long long)resumed.presentCalls, resumed.dxgiPresentCount, validation, errors);
        }
        REQUIRE(errors == 0);
        std::puts("AMD native SR readback, Anti-Lag, frame generation/interpolated DXGI presents and disable: PASS");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "AMD frame generation failed: %s\n", error.what());
        return 1;
    }
}
