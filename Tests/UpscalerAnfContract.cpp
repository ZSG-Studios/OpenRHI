#include "SharedExternal.h"
#include "UpscalerInterface.h"

#include <cstdio>
#include <cstdlib>
#if defined(_WIN32)
#    include <malloc.h>
#endif

using namespace openrhi;

// Exercise the production adapter with a deterministic SDK function table, without a GPU
#include "UpscalerInterface.hpp"

static uint64_t g_instanceStorage, g_techniqueStorage, g_libraryStorage;
static uint32_t g_instances, g_techniques, g_unloads, g_dispatches, g_allocations, g_invalid;
static AnfResult g_createResult = ANF_RESULT_SUCCESS;
static AnfResult g_instanceResult = ANF_RESULT_SUCCESS;
static AnfResult g_dispatchResult = ANF_RESULT_SUCCESS;
static bool g_missingLibrary, g_partialTechnique, g_missingFunctions, g_missingSymbol, g_badRequirements, g_badResources, g_failAllocation;

void DeviceBase::ReportMessage(Message message, Result, const char*, uint32_t, const char*, ...) const {
    g_invalid += message == Message::ERROR;
}

static AnfInstance Instance() {
    return (AnfInstance)&g_instanceStorage;
}

static AnfTechnique Technique() {
    return (AnfTechnique)&g_techniqueStorage;
}

static AnfResult CreateInstance(const AnfInstanceCreateInfo* create, AnfInstance* instance) {
    g_invalid += create->clientApi != ANF_CLIENT_API_VULKAN;
    *instance = Instance();
    g_instances++;

    return g_instanceResult;
}

static AnfResult DestroyInstance(AnfInstance instance) {
    g_invalid += instance != Instance() || g_techniques != 0;
    g_instances--;

    return ANF_RESULT_SUCCESS;
}

static AnfResult SetAdapter(AnfInstance instance, const AnfStructHeader* header) {
    const auto& adapter = *(const AnfAdapterInfoVulkan*)header;
    g_invalid += instance != Instance() || header->type != ANF_STYPE_ADAPTER_INFO_VULKAN || !adapter.instance || !adapter.device || !adapter.physicalDevice;

    return ANF_RESULT_SUCCESS;
}

static AnfResult IsSupported(AnfInstance instance, AnfTechniqueId technique, const AnfStructHeader*) {
    g_invalid += instance != Instance() || technique != ANF_TECHNIQUE_ID_SR_TEMPORAL;

    return ANF_RESULT_SUCCESS;
}

static AnfResult QueryRequirements(AnfInstance, AnfTechniqueId, const AnfTechniqueRequirements** output) {
    static const char* extensions[] = {"VK_KHR_external_memory_fd", "VK_KHR_external_semaphore_fd"};
    static AnfTechniqueVulkanApiRequirements vk = {{ANF_STYPE_TECHNIQUE_VULKAN_API_REQUIREMENTS}, 0, nullptr, 2, extensions, VK_QUEUE_COMPUTE_BIT};
    static AnfSRRequirements sr = {{ANF_STYPE_SR_REQUIREMENTS}, 1u << ANF_SR_QUALITY_MODE_PERFORMANCE};
    static const AnfResourceLabel resources[] = {ANF_RESOURCE_LABEL_DEPTH, ANF_RESOURCE_LABEL_MOTION_VECTORS, ANF_RESOURCE_LABEL_INPUT_COLOR, ANF_RESOURCE_LABEL_OUTPUT_COLOR};
    static AnfTechniqueRequirements requirements = {};
    requirements.header.type = ANF_STYPE_TECHNIQUE_REQUIREMENTS;
    requirements.pRequiredResources = g_badRequirements ? nullptr : resources;
    requirements.numRequiredResources = 4;
    requirements.pClientApiRequirements = &vk.header;
    requirements.pTechniqueGroupRequirements = &sr.header;
    *output = &requirements;

    return ANF_RESULT_SUCCESS;
}

static AnfResult QueryResource(AnfInstance, AnfTechniqueId, AnfResourceLabel label, const AnfResourceRequirements** output) {
    static const AnfFormat formats[] = {ANF_FORMAT_D32_FLOAT, ANF_FORMAT_R16G16_FLOAT, ANF_FORMAT_R8G8B8A8_UNORM, ANF_FORMAT_R16G16B16A16_FLOAT};
    static AnfResourceVulkanApiRequirements vk = {};
    static AnfResourceRequirements resource = {};
    vk.header.type = ANF_STYPE_RESOURCE_VULKAN_API_REQUIREMENTS;
    vk.imageUsageFlags = label == ANF_RESOURCE_LABEL_OUTPUT_COLOR ? VK_IMAGE_USAGE_STORAGE_BIT : VK_IMAGE_USAGE_SAMPLED_BIT;
    vk.externalMemoryType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    resource.header = {}; // Matches the pinned SDK's returned base resource header
    resource.resourceType = ANF_RESOURCE_TYPE_TEXTURE2D;
    resource.pSupportedFormats = g_badResources ? nullptr : &formats[uint32_t(label)];
    resource.numSupportedFormats = 1;
    resource.pClientApiRequirements = &vk.header;
    *output = &resource;

    return ANF_RESULT_SUCCESS;
}

static AnfResult CreateTechnique(AnfInstance instance, const AnfTechniqueCreateInfo* create, AnfTechnique* output) {
    const auto& sr = *(const AnfSRCreateInfo*)create->pTechniqueGroupCreateInfo;
    g_invalid += instance != Instance() || create->flags != 0 || create->maxInFlight != 3 || sr.inputSize.width != 960 || sr.inputSize.height != 540 || sr.outputSize.width != 1920 || sr.outputSize.height != 1080;
    *output = (g_createResult == ANF_RESULT_SUCCESS || g_partialTechnique) ? Technique() : nullptr;

    if (*output)
        g_techniques++;

    return g_createResult;
}

static AnfResult DestroyTechnique(AnfInstance instance, AnfTechnique technique) {
    g_invalid += instance != Instance() || technique != Technique() || g_instances != 1;
    g_techniques--;

    return ANF_RESULT_SUCCESS;
}

static AnfResult DispatchTechnique(AnfTechnique technique, const AnfTechniqueDispatchInfo* dispatch) {
    g_invalid += technique != Technique() || dispatch->numResources != 4 || !dispatch->commandList || dispatch->header.pNext != nullptr;
    const auto& sr = *(const AnfSRDispatch*)dispatch->pTechniqueGroupDispatchInfo;
    g_invalid += sr.jitterOffset.x != -0.25f || sr.jitterOffset.y != 0.375f;
    g_invalid += bool(dispatch->reset) != (g_dispatches == 0);

    for (uint32_t i = 0; i < 4; i++) {
        const auto& parameter = dispatch->pResources[i];
        const auto& resource = parameter.resourceDesc;
        const AnfFormat expected[] = {ANF_FORMAT_D32_FLOAT, ANF_FORMAT_R16G16_FLOAT, ANF_FORMAT_R8G8B8A8_UNORM, ANF_FORMAT_R16G16B16A16_FLOAT};
        g_invalid += uint32_t(parameter.resourceLabel) != i || resource.format != expected[i] || resource.resource != (AnfClientResource)(uintptr_t)(100 + i);
        g_invalid += resource.textureDims.width != (i == 3 ? 1920u : 960u) || resource.textureDims.height != (i == 3 ? 1080u : 540u) || resource.textureDims.mipCount != 1 || resource.textureDims.numArrayLayers != 1;
        g_invalid += resource.layout != uint32_t(i == 3 ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    g_dispatches++;

    return g_dispatchResult;
}

static void GetFunctions(AnfFunctions* functions) {
    *functions = {};
    functions->CreateInstance = CreateInstance;
    functions->DestroyInstance = DestroyInstance;
    functions->SetInstanceClientAPIAdapterInfo = SetAdapter;
    functions->IsTechniqueSupported = IsSupported;
    functions->QueryTechniqueRequirements = QueryRequirements;
    functions->QueryTechniqueResourceRequirements = QueryResource;
    functions->CreateTechnique = CreateTechnique;
    functions->DestroyTechnique = DestroyTechnique;
    functions->DispatchTechnique = DispatchTechnique;

    if (g_missingFunctions)
        functions->DestroyInstance = nullptr;
}

Library* openrhi::LoadSharedLibrary(const char*) {
    return g_missingLibrary ? nullptr : (Library*)&g_libraryStorage;
}

void* openrhi::GetSharedLibraryFunction(Library&, const char*) {
    return g_missingSymbol ? nullptr : (void*)GetFunctions;
}

void openrhi::UnloadSharedLibrary(Library&) {
    g_unloads++;
}

static VKHandle OPENRHI_CALL NativeDevice(const Device&) {
    return (VKHandle)uintptr_t(1);
}

OPENRHI_API Result OPENRHI_CALL openrhi::openrhiGetInterface(const Device&, const char*, size_t size, void* table) {
    if (size != sizeof(WrapperVKInterface))
        return Result::UNSUPPORTED;

    auto& wrapper = *(WrapperVKInterface*)table;
    wrapper = {};
    wrapper.GetInstanceVK = NativeDevice;
    wrapper.GetPhysicalDeviceVK = NativeDevice;

    return Result::SUCCESS;
}

struct TestDevice final : DeviceBase {
    TestDevice(const AllocationCallbacks& allocations) : DeviceBase({}, allocations) {
        desc.graphicsAPI = GraphicsAPI::VK;
        desc.features.adrenoNeuralFusion = true;
    }

    const DeviceDesc& GetDesc() const override {
        return desc;
    }

    void Destruct() override {
    }

    DeviceDesc desc = {};
};

struct TestTexture {
    TextureDesc desc;
    uint64_t native;
};

static void* OPENRHI_CALL AllocateTest(void*, size_t size, size_t alignment) {
    g_allocations++;

    if (g_failAllocation)
        return nullptr;

#if defined(_WIN32)
    return _aligned_malloc(size, alignment);
#else
    void* memory = nullptr;

    return posix_memalign(&memory, alignment, size) == 0 ? memory : nullptr;
#endif
}

static void OPENRHI_CALL FreeTest(void*, void* memory) {
#if defined(_WIN32)
    _aligned_free(memory);
#else
    free(memory);
#endif
}

static bool Check(bool condition, const char* message) {
    if (!condition)
        std::printf("FAIL: %s\n", message);

    return condition;
}

int main() {
    UpscalerRequirementsVK requirements = {};

    if (!Check(GetUpscalerRequirementsVK(UpscalerType::ANF, requirements) == Result::SUCCESS && requirements.deviceExtensionNum == 2 && requirements.resources[3].formats[0] == Format::D32_SFLOAT && g_instances == 0, "requirements and query cleanup"))
        return 1;

    g_missingLibrary = true;

    if (!Check(GetUpscalerRequirementsVK(UpscalerType::ANF, requirements) == Result::UNSUPPORTED && requirements.deviceExtensionNum == 0, "missing SDK reports unsupported with cleared output"))
        return 1;

    g_missingLibrary = false;
    g_missingSymbol = true;

    if (!Check(GetUpscalerRequirementsVK(UpscalerType::ANF, requirements) == Result::UNSUPPORTED && g_instances == 0, "missing SDK export"))
        return 1;

    g_missingSymbol = false;
    g_missingFunctions = true;

    if (!Check(GetUpscalerRequirementsVK(UpscalerType::ANF, requirements) == Result::UNSUPPORTED && g_instances == 0, "incomplete SDK function table"))
        return 1;

    g_missingFunctions = false;
    g_instanceResult = ANF_RESULT_ERROR_MEMORY;

    if (!Check(GetUpscalerRequirementsVK(UpscalerType::ANF, requirements) == Result::OUT_OF_MEMORY && g_instances == 0, "partial instance cleanup"))
        return 1;

    g_instanceResult = ANF_RESULT_SUCCESS;

    for (bool* failure : {&g_badRequirements, &g_badResources}) {
        *failure = true;

        if (!Check(GetUpscalerRequirementsVK(UpscalerType::ANF, requirements) == Result::UNSUPPORTED && requirements.deviceExtensionNum == 0 && g_instances == 0, "invalid native requirements clear public output"))
            return 1;

        *failure = false;
    }

    {
        Anf anf;

        if (!Check(anf.Initialize() == Result::SUCCESS, "support query instance"))
            return 1;

        const char* extensions[] = {"VK_KHR_external_memory_fd", "VK_KHR_external_semaphore_fd"};
        const VKExtensions enabled = {nullptr, 0, extensions, 2};

        if (!Check(!anf.IsSupported((VKHandle)1, (VKHandle)2, (VKHandle)3, {}), "available extensions must be enabled")
            || !Check(anf.IsSupported((VKHandle)1, (VKHandle)2, (VKHandle)3, enabled), "enabled device requirements"))
            return 1;
    }

    const AllocationCallbacks allocations = {AllocateTest, nullptr, FreeTest};
    TestDevice device(allocations);
    CoreInterface core = {};
    core.GetDeviceDesc = [](const Device& object) -> const DeviceDesc& { return ((const TestDevice&)object).GetDesc(); };
    core.GetDeviceNativeObject = [](const Device* object) -> void* { return (void*)object; };
    core.GetCommandBufferNativeObject = [](const CommandBuffer* object) -> void* { return (void*)object; };
    core.GetTextureDesc = [](const Texture& object) -> const TextureDesc& { return ((const TestTexture&)object).desc; };
    core.GetTextureNativeObject = [](const Texture* object) -> uint64_t { return ((const TestTexture*)object)->native; };
    UpscalerDesc desc = {};
    desc.type = UpscalerType::ANF;
    desc.mode = UpscalerMode::PERFORMANCE;
    desc.upscaleResolution = {1920, 1080};
    desc.maxInFlight = 3;
    g_failAllocation = true;
    {
        UpscalerImpl upscaler((Device&)device, core);

        if (!Check(upscaler.Create(desc) == Result::OUT_OF_MEMORY && g_instances == 0, "adapter allocation failure"))
            return 1;
    }
    g_failAllocation = false;
    TestTexture textures[4] = {};
    const Format formats[] = {Format::D32_SFLOAT, Format::RG16_SFLOAT, Format::RGBA8_UNORM, Format::RGBA16_SFLOAT};

    for (uint32_t i = 0; i < 4; i++) {
        textures[i].native = 100 + i;
        textures[i].desc.format = formats[i];
        textures[i].desc.width = i == 3 ? 1920 : 960;
        textures[i].desc.height = i == 3 ? 1080 : 540;
        textures[i].desc.layerNum = textures[i].desc.mipNum = 1;
    }

    struct Scenario {
        AnfResult result;
        bool partialTechnique;
    };

    for (const Scenario& scenario : {Scenario{ANF_RESULT_NOT_SUPPORTED, false}, Scenario{ANF_RESULT_NOT_SUPPORTED, true}, Scenario{ANF_RESULT_ERROR_MEMORY, false}, Scenario{ANF_RESULT_ERROR_MEMORY, true}, Scenario{ANF_RESULT_SUCCESS, false}}) {
        g_createResult = scenario.result;
        g_partialTechnique = scenario.partialTechnique;
        {
            UpscalerImpl upscaler((Device&)device, core);
            Result result = upscaler.Create(desc);

            if (!Check(result == ConvertAnfResult(scenario.result), "native creation result propagation"))
                return 1;

            if (result == Result::SUCCESS) {
                DispatchUpscaleDesc dispatch = {};
                dispatch.guides.upscaler.depth.texture = (Texture*)&textures[0];
                dispatch.guides.upscaler.mv.texture = (Texture*)&textures[1];
                dispatch.input.texture = (Texture*)&textures[2];
                dispatch.output.texture = (Texture*)&textures[3];
                dispatch.cameraJitter = {-0.25f, 0.375f};
                dispatch.flags = DispatchUpscaleBits::RESET_HISTORY;
                uint64_t command = 1;
                uint32_t allocationsBeforeDispatch = g_allocations;
                upscaler.CmdDispatchUpscale(*(CommandBuffer*)&command, dispatch);
                dispatch.flags = DispatchUpscaleBits::NONE;
                upscaler.CmdDispatchUpscale(*(CommandBuffer*)&command, dispatch);

                if (!Check(g_allocations == allocationsBeforeDispatch && g_dispatches == 2, "two dispatches with no adapter allocations"))
                    return 1;

                g_dispatchResult = ANF_RESULT_ERROR_INVALID_PARAMETER;
                upscaler.CmdDispatchUpscale(*(CommandBuffer*)&command, dispatch);
                g_dispatchResult = ANF_RESULT_SUCCESS;

                if (!Check(g_invalid == 1 && g_allocations == allocationsBeforeDispatch, "dispatch failure reported without adapter allocation"))
                    return 1;

                g_invalid = 0;
            }
        }

        if (!Check(g_instances == 0 && g_techniques == 0 && g_invalid == 0, "partial and complete SDK lifetime cleanup"))
            return 1;
    }

    std::printf("ANF adapter contract PASS: requirements, unsupported/error cleanup, resource/format/layout mapping, jitter, reset, maxInFlight; dispatches=%u unloads=%u (SDK stubs, no GPU execution)\n", g_dispatches, g_unloads);

    return 0;
}
