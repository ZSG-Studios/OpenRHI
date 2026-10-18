#include "OpenRHI.h"

#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIHelper.h"

#include <cstdio>

extern "C" int OpenRhiCheckPublicHelpers(void);

int main() {
    if (OpenRhiCheckPublicHelpers())
        return 1;

    const openrhi::Format format = openrhi::Format::RGBA8_UNORM;
    const uint32_t dxgi = openrhi::openrhiConvertOpenRHIFormatToDXGI(format);
    const uint32_t vk = openrhi::openrhiConvertOpenRHIFormatToVK(format);

    if ((dxgi && openrhi::openrhiConvertDXGIFormatToOpenRHI(dxgi) != format) || (vk && openrhi::openrhiConvertVKFormatToOpenRHI(vk) != format))
        return 2;

#if OPENRHI_TEST_NONE
    openrhi::DeviceCreationDesc creation = {};
    creation.graphicsAPI = openrhi::GraphicsAPI::NONE;
    openrhi::Device* device = nullptr;

    if (openrhi::openrhiCreateDevice(creation, device) != openrhi::Result::SUCCESS)
        return 3;

    bool ok = true;
    const char* names[] = {"CoreInterface", "openrhi::CoreInterface", "OpenRhiCoreInterface"};

    for (const char* name : names) {
        openrhi::CoreInterface core = {};
        ok &= openrhi::openrhiGetInterface(*device, name, sizeof(core), &core) == openrhi::Result::SUCCESS;
        ok &= core.GetDeviceDesc != nullptr && core.CreateBuffer != nullptr && core.DestroyBuffer != nullptr;
    }

    openrhi::openrhiDestroyDevice(device);

    if (!ok)
        return 4;
#endif
    std::puts("OpenRHI C/C++ public names, native format conversions and interface lookup: PASS");

    return 0;
}
