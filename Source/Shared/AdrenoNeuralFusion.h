// OpenRHI adapter for the unmodified Qualcomm ANF public SDK

#pragma once

#include <anf.h>
#include <anf_types_vk.h>

struct Anf {
    ~Anf();
    openrhi::Result Initialize();
    openrhi::Result QueryRequirements(openrhi::UpscalerRequirementsVK& output);
    bool IsSupported(VKHandle vkInstance, VKHandle physicalDevice, VKHandle device, const openrhi::VKExtensions& enabledExtensions);

private:
    friend struct openrhi::UpscalerImpl;
    openrhi::Library* library = nullptr;
    Lock sdkLock = {};
    AnfFunctions functions = {};
    AnfInstance instance = nullptr;
    AnfTechnique technique = nullptr;
    openrhi::UpscalerResourceRequirementsVK resources[4] = {};
};
