// Compile the public facade as C in addition to the C++ runtime client
#include "OpenRHI.h"

#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIHelper.h"
#include "Extensions/OpenRHIWrapperD3D12.h"
#include "Extensions/OpenRHIWrapperMetal.h"
#include "Extensions/OpenRHIWrapperVK.h"

_Static_assert(OpenRhiMember(GraphicsAPI, NONE) == (1 << 0), "NONE bit changed");
_Static_assert(OpenRhiMember(GraphicsAPI, D3D12) == (1 << 2), "D3D12 bit changed");
_Static_assert(OpenRhiMember(GraphicsAPI, VK) == (1 << 3), "VK bit changed");
_Static_assert(OpenRhiMember(GraphicsAPI, METAL) == (1 << 5), "METAL bit changed");

int OpenRhiCheckPublicHelpers(void) {
    const OpenRhiFormat format = OpenRhiMember(Format, RGBA8_UNORM);
    uint32_t dxgi = openrhiConvertOpenRHIFormatToDXGI(format);
    uint32_t vk = openrhiConvertOpenRHIFormatToVK(format);
    const OpenRhiFormatProps* props = openrhiGetFormatProps(format);

    if (!props || props->format != format)
        return 1;

    if (dxgi && openrhiConvertDXGIFormatToOpenRHI(dxgi) != format)
        return 2;

    if (vk && openrhiConvertVKFormatToOpenRHI(vk) != format)
        return 3;

    return 0;
}
