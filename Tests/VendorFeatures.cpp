#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <d3d12.h>
#include <windows.h>
#include <wrl/client.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "OpenRHI.h"
#include "OpenRHIAgilitySDK.h"

#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIVendorFeatures.h"
#include "dx12/ffx_api_dx12.h"
#include "ffx_denoiser.h"
#include "ffx_radiancecache.h"
#if OPENRHI_ENABLE_NRC_SDK
#    include "NrcD3d12.h"
#endif

using namespace openrhi;
using Microsoft::WRL::ComPtr;
#define REQUIRE(x) \
    do { \
        if (!(x)) \
            throw std::runtime_error(#x); \
    } while (false)

static uint32_t errors;

static void OPENRHI_CALL MessageCallback(Message type, const char*, uint32_t, const char* text, void*) {
    errors += type == Message::ERROR;
    std::fprintf(stderr, "%s\n", text);
}

static void OPENRHI_CALL NoAbort(void*) {
}

struct Fixture {
    Device* device = nullptr;
    CoreInterface core = {};
    FidelityFXInterface ffx = {};
    Queue* queue = nullptr;
    CommandAllocator* allocator = nullptr;
    CommandBuffer* command = nullptr;
    Fence* fence = nullptr;
    uint64_t value = 0;
    FidelityFXContext* context = nullptr;
    ID3D12Device* nativeDevice = nullptr;
    ID3D12GraphicsCommandList* nativeCommand = nullptr;
    ComPtr<ID3D12DescriptorHeap> heap, cpuHeap;
    std::vector<ComPtr<ID3D12Resource>> resources;
    uint32_t descriptorIndex = 0;

    ~Fixture() {
        if (!device)
            return;
        if (core.DeviceWaitIdle)
            core.DeviceWaitIdle(device);
        if (context)
            ffx.DestroyFidelityFXContext(context);
        resources.clear();
        if (fence)
            core.DestroyFence(fence);
        if (command)
            core.DestroyCommandBuffer(command);
        if (allocator)
            core.DestroyCommandAllocator(allocator);
        openrhiDestroyDevice(device);
    }

    void Initialize(bool validation) {
        DeviceCreationDesc desc = {};
        desc.graphicsAPI = GraphicsAPI::D3D12;
        desc.enableOpenRHIValidation = validation;
        desc.enableGraphicsAPIValidation = true;
        desc.callbackInterface.MessageCallback = MessageCallback;
        desc.callbackInterface.AbortExecution = NoAbort;
        REQUIRE(openrhiCreateDevice(desc, device) == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(CoreInterface), &core) == Result::SUCCESS);
        REQUIRE(openrhiGetInterface(*device, OPENRHI_INTERFACE(FidelityFXInterface), &ffx) == Result::SUCCESS);
        const auto& adapter = core.GetDeviceDesc(*device).adapterDesc;
        std::printf("GPU=%s vendor=%u device=%u validation=%d\n", adapter.name, unsigned(adapter.vendor), adapter.deviceId, validation);
        std::printf("nativeDriverVersion=%llu uid=%llu:%llu\n", (unsigned long long)adapter.nativeDriverVersion, (unsigned long long)adapter.uid.high, (unsigned long long)adapter.uid.low);
        REQUIRE(core.GetQueue(*device, QueueType::GRAPHICS, 0, queue) == Result::SUCCESS);
        REQUIRE(core.CreateCommandAllocator(*queue, allocator) == Result::SUCCESS);
        REQUIRE(core.CreateCommandBuffer(*allocator, command) == Result::SUCCESS);
        REQUIRE(core.CreateFence(*device, 0, fence) == Result::SUCCESS);
        nativeDevice = (ID3D12Device*)core.GetDeviceNativeObject(device);
        nativeCommand = (ID3D12GraphicsCommandList*)core.GetCommandBufferNativeObject(command);
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = 128;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        REQUIRE(SUCCEEDED(nativeDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap))));
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        REQUIRE(SUCCEEDED(nativeDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cpuHeap))));
    }

    void Begin() {
        core.ResetCommandAllocator(*allocator);
        REQUIRE(core.BeginCommandBuffer(*command, nullptr) == Result::SUCCESS);
        ID3D12DescriptorHeap* h = heap.Get();
        nativeCommand->SetDescriptorHeaps(1, &h);
        descriptorIndex = 0;
    }

    void Submit() {
        REQUIRE(core.EndCommandBuffer(*command) == Result::SUCCESS);
        FenceSubmitDesc signal = {fence, ++value, StageBits::ALL};
        QueueSubmitDesc desc = {};
        desc.commandBuffers = &command;
        desc.commandBufferNum = 1;
        desc.signalFences = &signal;
        desc.signalFenceNum = 1;
        REQUIRE(core.QueueSubmit(*queue, desc) == Result::SUCCESS);
        core.Wait(*fence, value);
        REQUIRE(core.GetFenceValue(*fence) >= value);
    }

    void Barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after;
        nativeCommand->ResourceBarrier(1, &barrier);
    }

    ID3D12Resource* Texture(DXGI_FORMAT format, const float* clear, bool output = false) {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = desc.Height = 64;
        desc.DepthOrArraySize = desc.MipLevels = desc.SampleDesc.Count = 1;
        desc.Format = format;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_DEFAULT;
        ComPtr<ID3D12Resource> resource;
        REQUIRE(SUCCEEDED(nativeDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resource))));
        auto cpu = cpuHeap->GetCPUDescriptorHandleForHeapStart();
        auto visibleCPU = heap->GetCPUDescriptorHandleForHeapStart();
        auto gpu = heap->GetGPUDescriptorHandleForHeapStart();
        const auto offset = descriptorIndex++ * nativeDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        cpu.ptr += offset;
        gpu.ptr += offset;
        visibleCPU.ptr += offset;
        nativeDevice->CreateUnorderedAccessView(resource.Get(), nullptr, nullptr, cpu);
        nativeDevice->CreateUnorderedAccessView(resource.Get(), nullptr, nullptr, visibleCPU);
        nativeCommand->ClearUnorderedAccessViewFloat(gpu, cpu, resource.Get(), clear, 0, nullptr);
        if (!output)
            Barrier(resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        resources.push_back(resource);
        return resource.Get();
    }

    ID3D12Resource* Buffer(uint64_t bytes, uint32_t stride, const void* data = nullptr) {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = bytes;
        desc.Height = desc.DepthOrArraySize = desc.MipLevels = desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_DEFAULT;
        ComPtr<ID3D12Resource> resource;
        REQUIRE(SUCCEEDED(nativeDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource))));
        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        ComPtr<ID3D12Resource> upload;
        REQUIRE(SUCCEEDED(nativeDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload))));
        void* mapped = nullptr;
        REQUIRE(SUCCEEDED(upload->Map(0, nullptr, &mapped)));
        if (data)
            std::memcpy(mapped, data, size_t(bytes));
        else
            std::memset(mapped, 0, size_t(bytes));
        upload->Unmap(0, nullptr);
        nativeCommand->CopyBufferRegion(resource.Get(), 0, upload.Get(), 0, bytes);
        Barrier(resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        resources.push_back(upload);
        resources.push_back(resource);
        (void)stride;
        return resource.Get();
    }

    void CheckReadback(ID3D12Resource* resource, bool texture, bool clearedCounters = false) {
        auto desc = resource->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
        uint64_t bytes = desc.Width;
        if (texture)
            nativeDevice->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, &bytes);
        D3D12_RESOURCE_DESC buffer = {};
        buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        buffer.Width = bytes;
        buffer.Height = buffer.DepthOrArraySize = buffer.MipLevels = buffer.SampleDesc.Count = 1;
        buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_READBACK;
        ComPtr<ID3D12Resource> readback;
        REQUIRE(SUCCEEDED(nativeDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback))));
        Barrier(resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        if (texture) {
            D3D12_TEXTURE_COPY_LOCATION source = {};
            source.pResource = resource;
            source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            D3D12_TEXTURE_COPY_LOCATION dest = {};
            dest.pResource = readback.Get();
            dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            dest.PlacedFootprint = footprint;
            nativeCommand->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
        } else
            nativeCommand->CopyBufferRegion(readback.Get(), 0, resource, 0, bytes);
        Submit();
        void* mapped = nullptr;
        REQUIRE(SUCCEEDED(readback->Map(0, nullptr, &mapped)));
        if (clearedCounters) {
            REQUIRE(((const uint32_t*)mapped)[0] == 0 && ((const uint32_t*)mapped)[1] == 0);
        } else if (!texture) {
            const auto* values = (const float*)mapped;
            double sum = 0;
            for (size_t i = 0; i < bytes / 4; ++i) {
                REQUIRE(std::isfinite(values[i]) && values[i] >= 0.0f);
                sum += values[i];
            }
            REQUIRE(sum > 0);
            std::printf("Radiance-cache readback sum=%g\n", sum);
        } else if (desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT) {
            uint64_t sum = 0;
            for (uint32_t y = 0; y < 64; ++y) {
                auto* row = (const uint16_t*)((const uint8_t*)mapped + y * footprint.Footprint.RowPitch);
                for (uint32_t x = 0; x < 64 * 4; ++x) {
                    REQUIRE((row[x] & 0x7c00) != 0x7c00);
                    if (x % 4 != 3)
                        sum += row[x] & 0x7fff;
                }
            }
            REQUIRE(sum > 0);
            std::printf("RR lighting readback half-sum=%llu\n", (unsigned long long)sum);
        } else {
            // R8_UNORM outputs must retain a nonzero signal
            uint64_t sum = 0;
            for (uint32_t y = 0; y < 64; ++y)
                for (uint32_t x = 0; x < 64; ++x)
                    sum += ((const uint8_t*)mapped)[y * footprint.Footprint.RowPitch + x];
            REQUIRE(sum > 0);
            std::printf("RR readback sum=%llu\n", (unsigned long long)sum);
        }
        readback->Unmap(0, nullptr);
    }
};

static void SdkMessage(uint32_t, const wchar_t* text) {
    std::fwprintf(stderr, L"FFX: %ls\n", text);
}

static void QueryProviders(Fixture& f, FidelityFXEffect effect, uint64_t type) {
    FidelityFXContext* session = nullptr;
    FidelityFXDesc desc = {effect, nullptr};
    REQUIRE(f.ffx.CreateFidelityFXContext(*f.device, desc, session) == Result::SUCCESS);
    ffxConfigureDescGlobalDebug debug = {};
    debug.header.type = FFX_API_CONFIGURE_DESC_TYPE_GLOBALDEBUG;
    debug.effectId = type & FFX_API_EFFECT_MASK;
    debug.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_VERBOSE;
    debug.fpMessage = SdkMessage;
    std::printf("FFX debug configure=%d\n", int(f.ffx.ConfigureFidelityFX(*session, &debug)));
    uint64_t count = 0;
    ffxQueryDescGetVersions versions = {};
    versions.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    versions.createDescType = type;
    versions.device = f.nativeDevice;
    versions.outputCount = &count;
    const auto result = f.ffx.QueryFidelityFX(*session, &versions);
    std::printf("provider query effect=%u result=%d count=%llu\n", unsigned(effect), int(result), (unsigned long long)count);
    if (result == Result::SUCCESS && count) {
        std::vector<const char*> names(size_t(count), nullptr);
        versions.versionNames = names.data();
        REQUIRE(f.ffx.QueryFidelityFX(*session, &versions) == Result::SUCCESS);
        for (const auto name : names)
            std::printf("provider=%s\n", name ? name : "null");
    }
    f.ffx.DestroyFidelityFXContext(session);
}

static bool RayRegeneration(Fixture& f) {
    QueryProviders(f, FidelityFXEffect::RAY_REGENERATION, FFX_API_CREATE_CONTEXT_DESC_TYPE_DENOISER);
    ffxCreateContextDescDenoiser create = {};
    create.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_DENOISER;
    create.version = FFX_DENOISER_VERSION;
    create.maxRenderSize = {64, 64};
    create.signalFlags = 0x7f;
    create.flags = FFX_DENOISER_ENABLE_VALIDATION;
    FidelityFXDesc desc = {FidelityFXEffect::RAY_REGENERATION, &create};
    Result result = f.ffx.CreateFidelityFXContext(*f.device, desc, f.context);
    std::printf("Ray Regeneration creation=%d\n", int(result));
    if (result == Result::UNSUPPORTED)
        return false;
    REQUIRE(result == Result::SUCCESS);
    f.core.SetDebugName(f.context, "vendor feature context");
    f.Begin();
    const float zero[4] = {}, depth[4] = {1, 1, 1, 1}, normal[4] = {0.5f, 0.5f, 0.5f, 0};
    auto* linearDepth = f.Texture(DXGI_FORMAT_R32_FLOAT, depth);
    auto* motion = f.Texture(DXGI_FORMAT_R16G16B16A16_FLOAT, zero);
    auto* normals = f.Texture(DXGI_FORMAT_R10G10B10A2_UNORM, normal);
    auto* input = f.Texture(DXGI_FORMAT_R8_UNORM, depth);
    auto* output = f.Texture(DXGI_FORMAT_R8_UNORM, zero, true);
    auto* albedo = f.Texture(DXGI_FORMAT_R8G8B8A8_UNORM, depth);
    auto* radiance = f.Texture(DXGI_FORMAT_R16G16B16A16_FLOAT, depth);
    const float exposedShadow[4] = {65504, 0, 0, 0};
    auto* shadow = f.Texture(DXGI_FORMAT_R16_FLOAT, exposedShadow);
    ID3D12Resource* lightingOutputs[4] = {};
    for (auto& r : lightingOutputs)
        r = f.Texture(DXGI_FORMAT_R16G16B16A16_FLOAT, zero, true);
    auto* shadowOutput = f.Texture(DXGI_FORMAT_R8_UNORM, zero, true);
    auto* specularOcclusionOutput = f.Texture(DXGI_FORMAT_R8_UNORM, zero, true);
    f.Submit();
    for (uint32_t frame = 0; frame < 3; ++frame) {
        f.Begin();
        ffxDispatchDescDenoiserAmbientOcclusion ao = {};
        ao.header.type = FFX_API_DISPATCH_DESC_TYPE_DENOISER_AMBIENT_OCCLUSION;
        ao.signal.input = ffxApiGetResourceDX12(input);
        ao.signal.output = ffxApiGetResourceDX12(output, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
        ffxDispatchDescDenoiserDirectDiffuse dd = {};
        ffxDispatchDescDenoiserDirectSpecular ds = {};
        ffxDispatchDescDenoiserIndirectDiffuse id = {};
        ffxDispatchDescDenoiserIndirectSpecular is = {};
        ffxDispatchDescDenoiserDominantLight light = {};
        ffxDispatchDescDenoiserSpecularOcclusion so = {};
        dd.header = {FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE, &ds.header};
        ds.header = {FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR, &id.header};
        id.header = {FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_DIFFUSE, &is.header};
        is.header = {FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR, &light.header};
        light.header = {FFX_API_DISPATCH_DESC_TYPE_DENOISER_DOMINANT_LIGHT, &so.header};
        so.header.type = FFX_API_DISPATCH_DESC_TYPE_DENOISER_SPECULAR_OCCLUSION;
        ao.header.pNext = &dd.header;
        FfxApiDenoiserSignal* signals[] = {&dd.signal, &ds.signal, &id.signal, &is.signal};
        for (uint32_t i = 0; i < 4; ++i) {
            signals[i]->input = ffxApiGetResourceDX12(radiance);
            signals[i]->output = ffxApiGetResourceDX12(lightingOutputs[i], FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
        }
        light.signal.input = ffxApiGetResourceDX12(shadow);
        light.signal.output = ffxApiGetResourceDX12(shadowOutput, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
        light.direction = {0, -1, 0};
        light.emission = {1, 1, 1};
        light.angularRadius = 0.01f;
        so.signal.input = ffxApiGetResourceDX12(input);
        so.signal.output = ffxApiGetResourceDX12(specularOcclusionOutput, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
        ffxDispatchDescDenoiser dispatch = {};
        dispatch.header.type = FFX_API_DISPATCH_DESC_TYPE_DENOISER;
        dispatch.header.pNext = &ao.header;
        dispatch.linearDepth = ffxApiGetResourceDX12(linearDepth);
        dispatch.motionVectors = ffxApiGetResourceDX12(motion);
        dispatch.normals = ffxApiGetResourceDX12(normals);
        dispatch.diffuseAlbedo = dispatch.specularAlbedo = ffxApiGetResourceDX12(albedo);
        dispatch.motionVectorScale = {1, 1, 1};
        dispatch.linearDepthBounds = {0.01f, 100};
        dispatch.renderSize = {64, 64};
        dispatch.frameIndex = frame;
        dispatch.flags = frame == 0 ? FFX_DENOISER_DISPATCH_RESET : 0;
        dispatch.view.rows[0].x = dispatch.view.rows[1].y = dispatch.view.rows[2].z = dispatch.view.rows[3].w = 1;
        dispatch.projection = dispatch.view;
        REQUIRE(f.ffx.CmdDispatchFidelityFX(*f.command, *f.context, &dispatch) == Result::SUCCESS);
        REQUIRE(dispatch.commandList == nullptr);
        if (frame == 2)
            f.CheckReadback(output, true);
        else
            f.Submit();
    }
    for (auto* resource : lightingOutputs) {
        f.Begin();
        f.CheckReadback(resource, true);
    }
    for (auto* resource : {shadowOutput, specularOcclusionOutput}) {
        f.Begin();
        f.CheckReadback(resource, true);
    }
    f.ffx.DestroyFidelityFXContext(f.context);
    f.context = nullptr;
    return true;
}

static bool RadianceCache(Fixture& f) {
    QueryProviders(f, FidelityFXEffect::RADIANCE_CACHE, FFX_API_CREATE_CONTEXT_DESC_TYPE_RADIANCECACHE);
    ffxCreateContextDescRadianceCache create = {};
    create.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_RADIANCECACHE;
    create.version = FFX_RADIANCECACHE_VERSION;
    create.maxInferenceSampleCount = 4096;
    create.maxTrainingSampleCount = 16384;
    FidelityFXDesc desc = {FidelityFXEffect::RADIANCE_CACHE, &create};
    const auto result = f.ffx.CreateFidelityFXContext(*f.device, desc, f.context);
    std::printf("AMD radiance cache creation=%d\n", int(result));
    if (result == Result::UNSUPPORTED)
        return false;
    REQUIRE(result == Result::SUCCESS);
    f.core.SetDebugName(f.context, "vendor feature context");
    f.Begin();
    // SDK CacheInput is 11 floats; CacheOutput is 3 floats
    std::vector<float> queries(4096 * 11, 0.5f), training(16384 * 11, 0.5f), targets(16384 * 3, 1.0f);
    uint32_t counters[] = {4096, 16384};
    auto* prediction = f.Buffer(queries.size() * 4, 44, queries.data());
    auto* output = f.Buffer(4096 * 12, 12);
    auto* train = f.Buffer(training.size() * 4, 44, training.data());
    auto* target = f.Buffer(targets.size() * 4, 12, targets.data());
    auto* counter = f.Buffer(8, 4, counters);
    f.Submit();
    for (uint32_t frame = 0; frame < 3; ++frame) {
        f.Begin();
        auto buffer = [](ID3D12Resource* r, uint32_t stride) {
            auto resource = ffxApiGetResourceDX12(r, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
            resource.description.stride = stride;
            return resource;
        };
        ffxDispatchDescRadianceCache dispatch = {};
        dispatch.header.type = FFX_API_DISPATCH_DESC_TYPE_RADIANCECACHE;
        dispatch.predictionInputs = buffer(prediction, 44);
        dispatch.predictionOutputs = buffer(output, 12);
        dispatch.trainInputs = buffer(train, 44);
        dispatch.trainTargets = buffer(target, 12);
        dispatch.sampleCounters = buffer(counter, 4);
        dispatch.flags = FFX_RADIANCE_CACHE_DISPATCH_INFERENCE | FFX_RADIANCE_CACHE_DISPATCH_TRAINING;
        if (frame == 0)
            dispatch.flags |= FFX_RADIANCE_CACHE_RESET;
        if (frame == 2) {
            dispatch.flags |= FFX_RADIANCE_CACHE_CLEAR_ALL_COUNTERS | FFX_RADIANCE_CACHE_OVERRIDE_LEARNING_RATE | FFX_RADIANCE_CACHE_OVERRIDE_WEIGHT_SMOOTHING;
            dispatch.overrides.learningRate = 0.002f;
            dispatch.overrides.weightSmoothing = 0.99f;
        }
        REQUIRE(f.ffx.CmdDispatchFidelityFX(*f.command, *f.context, &dispatch) == Result::SUCCESS);
        REQUIRE(dispatch.commandList == nullptr);
        if (frame == 2) {
            f.CheckReadback(output, false);
            f.Begin();
            f.CheckReadback(counter, false, true);
        } else
            f.Submit();
    }
    f.ffx.DestroyFidelityFXContext(f.context);
    f.context = nullptr;
    return true;
}

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    try {
        uint32_t executed = 0;
        for (bool validation : {false, true}) {
            Fixture f;
            f.Initialize(validation);
            if (f.core.GetDeviceDesc(*f.device).adapterDesc.vendor != Vendor::AMD)
                return 77;
            executed += RayRegeneration(f);
            executed += RadianceCache(f);
            AntiLagInterface antiLag = {};
            REQUIRE(openrhiGetInterface(*f.device, OPENRHI_INTERFACE(AntiLagInterface), &antiLag) == Result::SUCCESS);
            AntiLagContext* context = nullptr;
            const auto result = antiLag.CreateAntiLagContext(*f.device, context);
            std::printf("Anti-Lag 2 creation=%d\n", int(result));
            REQUIRE(result == Result::SUCCESS);
            f.core.SetDebugName(context, "Anti-Lag 2 context");
            ComPtr<IDXGIFactory2> factory;
            REQUIRE(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));
            HWND window = CreateWindowExW(0, L"STATIC", L"OpenRHI Anti-Lag contract", WS_POPUP, 0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            REQUIRE(window);
            DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
            swapDesc.Width = swapDesc.Height = 64;
            swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapDesc.SampleDesc.Count = 1;
            swapDesc.BufferCount = 2;
            swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            ComPtr<IDXGISwapChain1> swap;
            REQUIRE(SUCCEEDED(factory->CreateSwapChainForHwnd((ID3D12CommandQueue*)f.core.GetQueueNativeObject(f.queue), window, &swapDesc, nullptr, nullptr, &swap)));
            REQUIRE(antiLag.AttachAntiLagSwapChain(*context, swap.Get()) == Result::SUCCESS);
            REQUIRE(antiLag.UpdateAntiLag(*context, true, 144) == Result::SUCCESS);
            REQUIRE(antiLag.UpdateAntiLag(*context, true, 0) == Result::SUCCESS);
            const GUID privateGuid = {0x5083ae5b, 0x8070, 0x4fca, {0x8e, 0xe5, 0x35, 0x82, 0xdd, 0x36, 0x7d, 0x13}};

            struct PrivateData {
                void* context;
                bool enabled;
            } data = {};

            UINT dataSize = sizeof(data);
            REQUIRE(SUCCEEDED(swap->GetPrivateData(privateGuid, &dataSize, &data)) && data.context && data.enabled);
            REQUIRE(antiLag.MarkAntiLagEndOfRendering(*context) == Result::SUCCESS);
            REQUIRE(antiLag.SetAntiLagFrameType(*context, true) == Result::SUCCESS);
            REQUIRE(antiLag.SetAntiLagFrameType(*context, false) == Result::SUCCESS);
            REQUIRE(antiLag.UpdateAntiLag(*context, false, 0) == Result::SUCCESS);
            dataSize = sizeof(data);
            REQUIRE(SUCCEEDED(swap->GetPrivateData(privateGuid, &dataSize, &data)) && !data.enabled);
            REQUIRE(antiLag.AttachAntiLagSwapChain(*context, nullptr) == Result::SUCCESS);
            REQUIRE(antiLag.AttachAntiLagSwapChain(*context, swap.Get()) == Result::SUCCESS);
            antiLag.DestroyAntiLagContext(context);
            dataSize = sizeof(data);
            REQUIRE(swap->GetPrivateData(privateGuid, &dataSize, &data) == DXGI_ERROR_NOT_FOUND);
            swap.Reset();
            DestroyWindow(window);
            std::puts("Anti-Lag 2 update/limiter/frame types/private-data attachment and cleanup: PASS");
        }
        REQUIRE(errors == 0);
        std::printf("AMD native feature dispatches: %u/4 paths; validation errors=%u\n", executed, errors);
        return executed == 4 ? 0 : 77;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
}
