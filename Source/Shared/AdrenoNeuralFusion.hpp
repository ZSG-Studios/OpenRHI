// OpenRHI adapter for the unmodified Qualcomm ANF public SDK

#include <cstdio>

static inline void AnfLogMessage(AnfLogLevel level, const char* message) {
    std::fprintf(stderr, "ANF[%u] %s\n", (uint32_t)level, message ? message : "");
}

static inline Result ConvertAnfResult(AnfResult result) {
    switch (result) {
        case ANF_RESULT_SUCCESS:
            return Result::SUCCESS;
        case ANF_RESULT_NOT_SUPPORTED:
            return Result::UNSUPPORTED;
        case ANF_RESULT_ERROR_INVALID_PARAMETER:
        case ANF_RESULT_ERROR_UNKNOWN_STYPE:
            return Result::INVALID_ARGUMENT;
        case ANF_RESULT_ERROR_MEMORY:
            return Result::OUT_OF_MEMORY;
        default:
            return Result::FAILURE;
    }
}

static inline Format ConvertAnfFormat(AnfFormat format) {
    switch (format) {
        case ANF_FORMAT_R8G8B8A8_UNORM:
            return Format::RGBA8_UNORM;
        case ANF_FORMAT_B10G11R11_UFLOAT:
            return Format::R11_G11_B10_UFLOAT;
        case ANF_FORMAT_D32_FLOAT:
            return Format::D32_SFLOAT;
        case ANF_FORMAT_D24S8_UNORM:
            return Format::D24_UNORM_S8_UINT;
        case ANF_FORMAT_R16G16_FLOAT:
            return Format::RG16_SFLOAT;
        case ANF_FORMAT_R16G16B16A16_FLOAT:
            return Format::RGBA16_SFLOAT;
        default:
            return Format::UNKNOWN;
    }
}

static inline AnfFormat ConvertToAnfFormat(Format format) {
    for (uint32_t value = ANF_FORMAT_R8G8B8A8_UNORM; value <= ANF_FORMAT_R16G16B16A16_FLOAT; value++) {
        if (ConvertAnfFormat((AnfFormat)value) == format)
            return (AnfFormat)value;
    }

    return ANF_FORMAT_UNKNOWN;
}

Anf::~Anf() {
    if (technique) {
        AnfResult result = functions.DestroyTechnique(instance, technique);
        MaybeUnused(result);
        OPENRHI_CHECK(result == ANF_RESULT_SUCCESS, "ANF DestroyTechnique failed");
    }

    if (instance) {
        AnfResult result = functions.DestroyInstance(instance);
        MaybeUnused(result);
        OPENRHI_CHECK(result == ANF_RESULT_SUCCESS, "ANF DestroyInstance failed");
    }

    if (library)
        UnloadSharedLibrary(*library);
}

Result Anf::Initialize() {
    library = LoadSharedLibrary("libanf.so");

    if (!library)
        return Result::UNSUPPORTED;

    auto getFunctions = (void (*)(AnfFunctions*))GetSharedLibraryFunction(*library, "GetAnfFunctions");

    if (!getFunctions)
        return Result::UNSUPPORTED;

    getFunctions(&functions);

    if (!functions.CreateInstance || !functions.DestroyInstance || !functions.SetInstanceClientAPIAdapterInfo || !functions.IsTechniqueSupported || !functions.QueryTechniqueRequirements || !functions.QueryTechniqueResourceRequirements || !functions.CreateTechnique || !functions.DestroyTechnique || !functions.DispatchTechnique)
        return Result::UNSUPPORTED;

    AnfInstanceCreateInfo create = {};
    create.header.type = ANF_STYPE_INSTANCE_CREATE_INFO;
    create.clientApi = ANF_CLIENT_API_VULKAN;
    create.logLevel = ANF_LOG_LEVEL_ERROR;
    create.logMessageCallback = AnfLogMessage;

    return ConvertAnfResult(functions.CreateInstance(&create, &instance));
}

Result Anf::QueryRequirements(UpscalerRequirementsVK& output) {
    const AnfTechniqueRequirements* requirements = nullptr;
    AnfResult result = functions.QueryTechniqueRequirements(instance, ANF_TECHNIQUE_ID_SR_TEMPORAL, &requirements);

    if (result != ANF_RESULT_SUCCESS)
        return ConvertAnfResult(result);

    if (!requirements || requirements->header.type != ANF_STYPE_TECHNIQUE_REQUIREMENTS || !requirements->pClientApiRequirements || requirements->pClientApiRequirements->type != ANF_STYPE_TECHNIQUE_VULKAN_API_REQUIREMENTS || requirements->numRequiredResources != 4 || !requirements->pRequiredResources || !requirements->pTechniqueGroupRequirements || requirements->pTechniqueGroupRequirements->type != ANF_STYPE_SR_REQUIREMENTS)
        return Result::UNSUPPORTED;

    const auto& sr = *(const AnfSRRequirements*)requirements->pTechniqueGroupRequirements;

    if (!(sr.supportedQualityModes & (1u << ANF_SR_QUALITY_MODE_PERFORMANCE)))
        return Result::UNSUPPORTED;

    const auto& native = *(const AnfTechniqueVulkanApiRequirements*)requirements->pClientApiRequirements;

    if (native.numInstanceExtensions > GetCountOf(output.instanceExtensions) || native.numDeviceExtensions > GetCountOf(output.deviceExtensions))
        return Result::UNSUPPORTED;

    if ((native.numInstanceExtensions && !native.ppInstanceExtensions) || (native.numDeviceExtensions && !native.ppDeviceExtensions))
        return Result::UNSUPPORTED;

    output = {};
    output.instanceExtensionNum = native.numInstanceExtensions;
    output.deviceExtensionNum = native.numDeviceExtensions;
    output.queueFlags = native.techniqueQueueFlags;

    for (uint32_t i = 0; i < native.numInstanceExtensions; i++) {
        if (!native.ppInstanceExtensions[i] || strlen(native.ppInstanceExtensions[i]) >= sizeof(output.instanceExtensions[i]))
            return Result::UNSUPPORTED;

        strcpy(output.instanceExtensions[i], native.ppInstanceExtensions[i]);
    }

    for (uint32_t i = 0; i < native.numDeviceExtensions; i++) {
        if (!native.ppDeviceExtensions[i] || strlen(native.ppDeviceExtensions[i]) >= sizeof(output.deviceExtensions[i]))
            return Result::UNSUPPORTED;

        strcpy(output.deviceExtensions[i], native.ppDeviceExtensions[i]);
    }

    constexpr AnfResourceLabel labels[] = {ANF_RESOURCE_LABEL_INPUT_COLOR, ANF_RESOURCE_LABEL_OUTPUT_COLOR, ANF_RESOURCE_LABEL_MOTION_VECTORS, ANF_RESOURCE_LABEL_DEPTH};

    for (uint32_t i = 0; i < GetCountOf(labels); i++) {
        bool required = false;

        for (uint32_t j = 0; j < requirements->numRequiredResources; j++)
            required |= requirements->pRequiredResources[j] == labels[i];

        if (!required)
            return Result::UNSUPPORTED;

        const AnfResourceRequirements* resource = nullptr;
        result = functions.QueryTechniqueResourceRequirements(instance, ANF_TECHNIQUE_ID_SR_TEMPORAL, labels[i], &resource);

        if (result != ANF_RESULT_SUCCESS)
            return ConvertAnfResult(result);

        // SDK 0.43.0 returns a zero base resource header, so validate the typed result payload and Vulkan extension
        if (!resource || resource->resourceType != ANF_RESOURCE_TYPE_TEXTURE2D || !resource->pClientApiRequirements || resource->pClientApiRequirements->type != ANF_STYPE_RESOURCE_VULKAN_API_REQUIREMENTS || !resource->numSupportedFormats || !resource->pSupportedFormats || resource->numSupportedFormats > GetCountOf(output.resources[i].formats))
            return Result::UNSUPPORTED;

        auto& target = output.resources[i];
        const auto& vk = *(const AnfResourceVulkanApiRequirements*)resource->pClientApiRequirements;
        target.imageUsageFlags = vk.imageUsageFlags;
        target.externalMemoryHandleTypes = vk.externalMemoryType;

        for (uint32_t j = 0; j < resource->numSupportedFormats; j++) {
            Format format = ConvertAnfFormat(resource->pSupportedFormats[j]);

            if (format != Format::UNKNOWN)
                target.formats[target.formatNum++] = format;
        }

        if (!target.formatNum)
            return Result::UNSUPPORTED;
    }

    return Result::SUCCESS;
}

bool Anf::IsSupported(VKHandle vkInstance, VKHandle physicalDevice, VKHandle device, const VKExtensions& enabledExtensions) {
    UpscalerRequirementsVK requirements = {};

    if (QueryRequirements(requirements) != Result::SUCCESS)
        return false;

    auto includes = [](const char* name, const char* const* names, uint32_t count) {
        if (!names)
            return false;

        for (uint32_t i = 0; i < count; i++) {
            if (names[i] && !strcmp(name, names[i]))
                return true;
        }

        return false;
    };

    for (uint32_t i = 0; i < requirements.instanceExtensionNum; i++) {
        if (!includes(requirements.instanceExtensions[i], enabledExtensions.instanceExtensions, enabledExtensions.instanceExtensionNum))
            return false;
    }

    for (uint32_t i = 0; i < requirements.deviceExtensionNum; i++) {
        if (!includes(requirements.deviceExtensions[i], enabledExtensions.deviceExtensions, enabledExtensions.deviceExtensionNum))
            return false;
    }

    AnfAdapterInfoVulkan adapter = {};
    adapter.header.type = ANF_STYPE_ADAPTER_INFO_VULKAN;
    adapter.instance = (VkInstance)vkInstance;
    adapter.physicalDevice = (VkPhysicalDevice)physicalDevice;
    adapter.device = (VkDevice)device;

    return functions.IsTechniqueSupported(instance, ANF_TECHNIQUE_ID_SR_TEMPORAL, &adapter.header) == ANF_RESULT_SUCCESS;
}

bool openrhi::IsAnfSupportedVK(VKHandle instance, VKHandle physicalDevice, VKHandle device, const VKExtensions& enabledExtensions) {
    Anf anf;

    return anf.Initialize() == Result::SUCCESS && anf.IsSupported(instance, physicalDevice, device, enabledExtensions);
}
