// Compile the public facade as C in addition to the C++ runtime client
#include "OpenRHI.h"

#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIHelper.h"
#include "Extensions/OpenRHIWrapperD3D12.h"
#include "Extensions/OpenRHIWrapperMetal.h"
#include "Extensions/OpenRHIWrapperVK.h"
#include "Extensions/OpenRHIUpscaler.h"
#include "Extensions/OpenRHIUpscalerVK.h"
#include "Extensions/OpenRHILowLatency.h"
#include "Extensions/OpenRHIVendorFeatures.h"
#include "Extensions/OpenRHIRmlUi.h"

_Static_assert(sizeof(OpenRhiRmlUiVertex) == 20, "RmlUi vertex ABI changed");

_Static_assert(OpenRhiMember(UpscalerType, ANF) == 8, "Upscaler values changed");
_Static_assert(OpenRhiMember(UpscalerResourceRole, MAX_NUM) == 4, "Update Vulkan resource requirements arrays");
_Static_assert(sizeof(((OpenRhiTextureViewDesc*)0)->layout) == sizeof(OpenRhiLayout), "Texture view layout must retain the C facade type");

_Static_assert(OpenRhiMember(GraphicsAPI, NONE) == (1 << 0), "NONE bit changed");
_Static_assert(OpenRhiMember(GraphicsAPI, D3D12) == (1 << 2), "D3D12 bit changed");
_Static_assert(OpenRhiMember(GraphicsAPI, VK) == (1 << 3), "VK bit changed");
_Static_assert(OpenRhiMember(GraphicsAPI, METAL) == (1 << 5), "METAL bit changed");

int OpenRhiCheckPublicHelpers(void) {
    OpenRhiUpscalerRequirementsVK upscalerRequirements = {0};

    if (openrhiGetUpscalerRequirementsVK(OpenRhiMember(UpscalerType, NIS), &upscalerRequirements) != OpenRhiMember(Result, UNSUPPORTED) || upscalerRequirements.deviceExtensionNum != 0)
        return 1;

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
