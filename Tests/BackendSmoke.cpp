// OpenRHI backend integration smoke test
#include "OpenRHI.h"

#if OPENRHI_ENABLE_AGILITY_SDK_SUPPORT
#    include "OpenRHIAgilitySDK.h"
#endif

#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIHelper.h"
#include "Extensions/OpenRHIWrapperD3D12.h"
#include "Extensions/OpenRHIWrapperMetal.h"
#include "Extensions/OpenRHIWrapperVK.h"

#include <cstdio>
#include <cstring>
#include <vector>

static int Run(openrhi::GraphicsAPI api, bool validation) {
    openrhi::DeviceCreationDesc desc = {};
    desc.graphicsAPI = api;
    desc.enableOpenRHIValidation = validation;
    openrhi::DeviceCompatibilityReport compatibility = {};
    desc.compatibilityReport = &compatibility;


    std::vector<openrhi::AdapterDesc> adapters;
    if (api != openrhi::GraphicsAPI::NONE) {
        uint32_t count = 0;
        if (openrhi::openrhiEnumerateAdapters(nullptr, count) != openrhi::Result::SUCCESS)
            return 1;

        adapters.resize(count);
        if (openrhi::openrhiEnumerateAdapters(adapters.data(), count) != openrhi::Result::SUCCESS)
            return 1;

        for (uint32_t i = 0; i < count; i++) {
            if ((uint8_t(adapters[i].supportedGraphicsAPIs) & uint8_t(api)) != 0) {
                desc.adapterDesc = &adapters[i];
                break;
            }
        }

        if (!desc.adapterDesc) {
            std::printf("SKIP: backend has no compatible adapter\n");
            return 77;
        }
    }

    openrhi::Device* device = nullptr;
    if (openrhi::openrhiCreateDevice(desc, device) != openrhi::Result::SUCCESS)
        return 1;

    if (compatibility.result != openrhi::Result::SUCCESS || uint32_t(compatibility.missingCapabilities) != 0) {
        openrhi::openrhiDestroyDevice(device);
        return 1;
    }
    const auto& profile = *openrhi::openrhiGetDeviceProfile(*device);
    if (profile.revision != 1 || profile.tunedPoliciesEnabled || profile.recommendedWaveSize != 0) {
        openrhi::openrhiDestroyDevice(device);
        return 1;
    }
    openrhi::CoreInterface core = {};
    if (openrhi::openrhiGetInterface(*device, OPENRHI_INTERFACE(openrhi::CoreInterface), &core) != openrhi::Result::SUCCESS) {
        openrhi::openrhiDestroyDevice(device);
        return 1;
    }

    openrhi::Queue* queue = nullptr;
    openrhi::CommandAllocator* allocator = nullptr;
    openrhi::CommandBuffer* command = nullptr;
    openrhi::Fence* fence = nullptr;
    bool ok = core.GetDeviceDesc(*device).graphicsAPI == api;
    ok = ok && core.GetQueue(*device, openrhi::QueueType::GRAPHICS, 0, queue) == openrhi::Result::SUCCESS;
    ok = ok && core.CreateCommandAllocator(*queue, allocator) == openrhi::Result::SUCCESS;
    ok = ok && core.CreateCommandBuffer(*allocator, command) == openrhi::Result::SUCCESS;
    ok = ok && core.CreateFence(*device, 0, fence) == openrhi::Result::SUCCESS;
    ok = ok && core.BeginCommandBuffer(*command, nullptr) == openrhi::Result::SUCCESS;
    ok = ok && core.EndCommandBuffer(*command) == openrhi::Result::SUCCESS;

    if (ok) {
        openrhi::FenceSubmitDesc signal = {};
        signal.fence = fence;
        signal.value = 1;
        signal.stages = openrhi::StageBits::ALL;
        openrhi::QueueSubmitDesc submit = {};
        submit.commandBuffers = &command;
        submit.commandBufferNum = 1;
        submit.signalFences = &signal;
        submit.signalFenceNum = 1;
        ok = core.QueueSubmit(*queue, submit) == openrhi::Result::SUCCESS;

        if (ok) {
            core.Wait(*fence, 1);
            ok = api == openrhi::GraphicsAPI::NONE || core.GetFenceValue(*fence) >= 1;
        }
    }

    if (fence)
        core.DestroyFence(fence);

    if (command)
        core.DestroyCommandBuffer(command);

    if (allocator)
        core.DestroyCommandAllocator(allocator);

    openrhi::openrhiDestroyDevice(device);
    std::printf("%s validation=%d: %s\n", openrhi::openrhiGetGraphicsAPIString(api), validation, ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    const openrhi::GraphicsAPI apis[] = {openrhi::GraphicsAPI::NONE, openrhi::GraphicsAPI::D3D12, openrhi::GraphicsAPI::VK, openrhi::GraphicsAPI::METAL};
    for (openrhi::GraphicsAPI api : apis) {
        if (argc == 2 && std::strcmp(argv[1], openrhi::openrhiGetGraphicsAPIString(api)) == 0) {
            int result = Run(api, false);
            return result ? result : Run(api, true);
        }
    }

    return 1;
}
