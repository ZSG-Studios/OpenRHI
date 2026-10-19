// OpenRHI Vulkan upscaler resource and device requirements

#pragma once

#define OPENRHI_UPSCALER_VK_H 1

#include "OpenRHIUpscaler.h"
#include "OpenRHIWrapperVK.h"

OpenRhiNamespaceBegin

OpenRhiEnum(UpscalerResourceRole, uint8_t,
    INPUT_COLOR,
    OUTPUT_COLOR,
    MOTION_VECTORS,
    DEPTH
);

OpenRhiStruct(UpscalerResourceRequirementsVK) {
    OpenRhi(Format) formats[8];
    uint32_t formatNum;
    VKFlags imageUsageFlags;
    VKFlags externalMemoryHandleTypes; // create/export native memory with these handle types, then wrap with WrapperVKInterface
};

OpenRhiStruct(UpscalerRequirementsVK) {
    char instanceExtensions[8][256];
    uint32_t instanceExtensionNum;
    char deviceExtensions[8][256];
    uint32_t deviceExtensionNum;
    VKFlags queueFlags; // every dispatch queue family must support all bits
    OpenRhi(UpscalerResourceRequirementsVK) resources[4]; // indexed by UpscalerResourceRole
};

// Explicit control-plane query before device/resource creation; all returned data is caller-owned
// Enable the returned extensions via DeviceCreationDesc::vkExtensions (or on an imported Vulkan device)
OPENRHI_API OpenRhi(Result) OPENRHI_CALL openrhiGetUpscalerRequirementsVK(OpenRhi(UpscalerType) type, OpenRhiOut OpenRhiRef(UpscalerRequirementsVK) requirements);

OpenRhiNamespaceEnd
