#include "OpenRHI.h"

#include "Extensions/OpenRHIDeviceCreation.h"
#include "Extensions/OpenRHIHelper.h"
#include "Extensions/OpenRHIUpscaler.h"
#include "Extensions/OpenRHIUpscalerVK.h"
#include "Extensions/OpenRHIWrapperVK.h"

#include <anf.h>
#include <anf_types_vk.h>
#include <dlfcn.h>
#include <vulkan/vulkan.h>
#include <cmath>
#include <cstdio>
#include <stdexcept>

using namespace openrhi;

static void Require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

static void OPENRHI_CALL MessageCallback(Message type, const char*, uint32_t, const char* message, void* context) {
    if (type == Message::ERROR)
        ++*(uint32_t*)context;

    std::fprintf(stderr, "%s\n", message);
}

static void OPENRHI_CALL NoAbort(void*) {
}

struct NativeImage {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    Texture* texture = nullptr;
    Descriptor* view = nullptr;
    Format format = Format::UNKNOWN;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct AnfFixture {
    Device* device = nullptr;
    CoreInterface core = {};
    WrapperVKInterface wrapper = {};
    UpscalerInterface upscaling = {};
    Upscaler* upscaler = nullptr;
    Queue* queue = nullptr;
    CommandAllocator* allocator = nullptr;
    CommandBuffer* command = nullptr;
    Fence* fence = nullptr;
    VkDevice native = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkBuffer readback = VK_NULL_HANDLE;
    VkDeviceMemory readbackMemory = VK_NULL_HANDLE;
    NativeImage images[4] = {};
    uint32_t errors = 0;

    ~AnfFixture() {
        if (!device)
            return;

        if (core.DeviceWaitIdle)
            core.DeviceWaitIdle(device);

        if (upscaler)
            upscaling.DestroyUpscaler(upscaler);

        for (auto& image : images) {
            if (image.view)
                core.DestroyDescriptor(image.view);

            if (image.texture)
                core.DestroyTexture(image.texture);

            if (image.image)
                vkDestroyImage(native, image.image, nullptr);

            if (image.memory)
                vkFreeMemory(native, image.memory, nullptr);
        }

        if (readback)
            vkDestroyBuffer(native, readback, nullptr);

        if (readbackMemory)
            vkFreeMemory(native, readbackMemory, nullptr);

        if (fence)
            core.DestroyFence(fence);

        if (command)
            core.DestroyCommandBuffer(command);

        if (allocator)
            core.DestroyCommandAllocator(allocator);

        openrhiDestroyDevice(device);
    }

    uint32_t MemoryType(uint32_t bits, VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties properties = {};
        vkGetPhysicalDeviceMemoryProperties(physical, &properties);

        for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
            if ((bits & (1u << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags)
                return i;
        }

        throw std::runtime_error("ANF memory type");
    }

    void CreateImage(uint32_t role, const UpscalerResourceRequirementsVK& requirement) {
        auto& image = images[role];
        image.width = role == 1 ? 1280 : 640;
        image.height = role == 1 ? 720 : 360;
        image.format = requirement.formats[0];

        for (uint32_t i = 0; i < requirement.formatNum; i++) {
            if (requirement.formats[i] == Format::RGBA8_UNORM || requirement.formats[i] == Format::D32_SFLOAT || requirement.formats[i] == Format::RG16_SFLOAT)
                image.format = requirement.formats[i];
        }

        std::printf("ANF_RESOURCE role=%u format=%u usage=0x%x external=0x%x\n", role, unsigned(image.format), requirement.imageUsageFlags, requirement.externalMemoryHandleTypes);
        const bool depth = role == 3;
        VkExternalMemoryImageCreateInfo external = {VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
        external.handleTypes = requirement.externalMemoryHandleTypes;
        VkImageCreateInfo create = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        create.pNext = external.handleTypes ? &external : nullptr;
        create.imageType = VK_IMAGE_TYPE_2D;
        create.format = (VkFormat)openrhiConvertOpenRHIFormatToVK(image.format);
        create.extent = {image.width, image.height, 1};
        create.mipLevels = create.arrayLayers = 1;
        create.samples = VK_SAMPLE_COUNT_1_BIT;
        create.tiling = VK_IMAGE_TILING_OPTIMAL;
        create.usage = requirement.imageUsageFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT | (role == 1 ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT : VK_IMAGE_USAGE_SAMPLED_BIT);
        create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Require(vkCreateImage(native, &create, nullptr, &image.image) == VK_SUCCESS, "ANF native image creation");
        VkMemoryRequirements memory = {};
        vkGetImageMemoryRequirements(native, image.image, &memory);
        VkExportMemoryAllocateInfo exportInfo = {VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO};
        exportInfo.handleTypes = external.handleTypes;
        VkMemoryDedicatedAllocateInfo dedicated = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
        dedicated.image = image.image;
        dedicated.pNext = external.handleTypes ? &exportInfo : nullptr;
        VkMemoryAllocateInfo allocation = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.pNext = &dedicated;
        allocation.allocationSize = memory.size;
        allocation.memoryTypeIndex = MemoryType(memory.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        Require(vkAllocateMemory(native, &allocation, nullptr, &image.memory) == VK_SUCCESS, "ANF external image allocation");
        Require(vkBindImageMemory(native, image.image, image.memory, 0) == VK_SUCCESS, "ANF image bind");
        TextureVKDesc imported = {};
        imported.vkImage = (VKNonDispatchableHandle)image.image;
        imported.vkFormat = create.format;
        imported.vkImageType = create.imageType;
        imported.vkImageUsageFlags = create.usage;
        imported.width = image.width;
        imported.height = image.height;
        imported.depth = imported.layerNum = imported.mipNum = imported.sampleNum = 1;
        Require(wrapper.CreateTextureVK(*device, imported, image.texture) == Result::SUCCESS, "ANF image import through OpenRHI");
        TextureViewDesc view = {};
        view.texture = image.texture;
        view.format = image.format;
        view.type = role == 1 ? TextureView::STORAGE_TEXTURE : TextureView::TEXTURE;
        view.planes = depth ? PlaneBits::DEPTH : PlaneBits::COLOR;
        Require(core.CreateTextureView(view, image.view) == Result::SUCCESS, "ANF OpenRHI descriptor");
    }

    void Transition(uint32_t index, VkImageLayout layout, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess) {
        auto& image = images[index];
        VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        barrier.srcStageMask = image.layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_2_NONE : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = image.layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_ACCESS_2_NONE : VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        barrier.dstStageMask = destinationStage;
        barrier.dstAccessMask = destinationAccess;
        barrier.oldLayout = image.layout;
        barrier.newLayout = layout;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.image;
        barrier.subresourceRange = {index == 3 ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkDependencyInfo dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency.imageMemoryBarrierCount = 1;
        dependency.pImageMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2((VkCommandBuffer)core.GetCommandBufferNativeObject(command), &dependency);
        image.layout = layout;
    }
};

static float Half(uint16_t value) {
    const uint32_t exponent = (value >> 10) & 31;
    const float fraction = float(value & 1023);

    if (exponent == 31)
        return NAN;

    const float magnitude = exponent ? std::ldexp(1.0f + fraction / 1024.0f, int(exponent) - 15) : std::ldexp(fraction, -24);

    return (value & 0x8000) ? -magnitude : magnitude;
}

int TestAnfDispatch() {
    UpscalerRequirementsVK requirements = {};
    Require(openrhiGetUpscalerRequirementsVK(UpscalerType::ANF, requirements) == Result::SUCCESS, "OpenRHI ANF requirements query");

    for (uint32_t i = 0; i < 4; i++) {
        const auto& resource = requirements.resources[i];
        std::printf("ANF_REQUIREMENT role=%u usage=0x%x external=0x%x format_count=%u\n", i, resource.imageUsageFlags, resource.externalMemoryHandleTypes, resource.formatNum);
    }

    AnfFixture fixture;
    const char* instanceExtensions[8] = {};
    const char* deviceExtensions[8] = {};

    for (uint32_t i = 0; i < requirements.instanceExtensionNum; i++)
        instanceExtensions[i] = requirements.instanceExtensions[i];

    for (uint32_t i = 0; i < requirements.deviceExtensionNum; i++)
        deviceExtensions[i] = requirements.deviceExtensions[i];

    DeviceCreationDesc create = {};
    create.graphicsAPI = GraphicsAPI::VK;
    create.enableOpenRHIValidation = true;
    create.enableGraphicsAPIValidation = true;
    create.callbackInterface = {MessageCallback, NoAbort, &fixture.errors};
    create.vkExtensions = {instanceExtensions, requirements.instanceExtensionNum, deviceExtensions, requirements.deviceExtensionNum};
    Require(openrhiCreateDevice(create, fixture.device) == Result::SUCCESS, "ANF OpenRHI device");
    Require(openrhiGetInterface(*fixture.device, OPENRHI_INTERFACE(CoreInterface), &fixture.core) == Result::SUCCESS, "ANF core interface");
    Require(openrhiGetInterface(*fixture.device, OPENRHI_INTERFACE(WrapperVKInterface), &fixture.wrapper) == Result::SUCCESS, "ANF Vulkan wrapper");
    Require(openrhiGetInterface(*fixture.device, OPENRHI_INTERFACE(UpscalerInterface), &fixture.upscaling) == Result::SUCCESS, "ANF upscaler interface");
    Require(fixture.upscaling.IsUpscalerSupported(*fixture.device, UpscalerType::ANF), "ANF device support");
    fixture.native = (VkDevice)fixture.core.GetDeviceNativeObject(fixture.device);
    fixture.physical = (VkPhysicalDevice)fixture.wrapper.GetPhysicalDeviceVK(*fixture.device);
    Require(fixture.core.GetQueue(*fixture.device, QueueType::GRAPHICS, 0, fixture.queue) == Result::SUCCESS, "ANF queue");
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(fixture.physical, &familyCount, nullptr);
    VkQueueFamilyProperties families[32] = {};
    Require(familyCount <= 32, "ANF queue count");
    vkGetPhysicalDeviceQueueFamilyProperties(fixture.physical, &familyCount, families);
    const uint32_t family = fixture.wrapper.GetQueueFamilyIndexVK(*fixture.queue);
    Require((families[family].queueFlags & requirements.queueFlags) == requirements.queueFlags, "ANF dispatch queue requirements");

    // Independent public-SDK diagnostic distinguishes driver support from adapter behavior
    void* library = dlopen("libanf.so", RTLD_NOW);
    Require(library != nullptr, "ANF diagnostic SDK load");
    auto getFunctions = (void (*)(AnfFunctions*))dlsym(library, "GetAnfFunctions");
    Require(getFunctions != nullptr, "ANF diagnostic function table");
    AnfFunctions api = {};
    getFunctions(&api);
    AnfInstanceCreateInfo instanceCreate = {};
    instanceCreate.header.type = ANF_STYPE_INSTANCE_CREATE_INFO;
    instanceCreate.clientApi = ANF_CLIENT_API_VULKAN;
    instanceCreate.logLevel = ANF_LOG_LEVEL_INFO;
    instanceCreate.logMessageCallback = [](AnfLogLevel level, const char* message) { std::printf("ANF_NATIVE[%u] %s\n", unsigned(level), message); };
    AnfInstance instance = nullptr;
    Require(api.CreateInstance(&instanceCreate, &instance) == ANF_RESULT_SUCCESS, "ANF diagnostic instance");
    VkQueue nativeQueue = (VkQueue)fixture.core.GetQueueNativeObject(fixture.queue);
    uint32_t queueFamily = family;
    AnfQueueInfoVulkan queueInfo = {};
    queueInfo.header.type = ANF_STYPE_QUEUE_INFO_VULKAN;
    queueInfo.numQueues = 1;
    queueInfo.pQueues = &nativeQueue;
    queueInfo.pQueueFamilyIndices = &queueFamily;
    AnfAdapterInfoVulkan adapter = {};
    adapter.header.type = ANF_STYPE_ADAPTER_INFO_VULKAN;
    adapter.header.pNext = &queueInfo.header;
    adapter.instance = (VkInstance)fixture.wrapper.GetInstanceVK(*fixture.device);
    adapter.physicalDevice = fixture.physical;
    adapter.device = fixture.native;
    Require(api.SetInstanceClientAPIAdapterInfo(instance, &adapter.header) == ANF_RESULT_SUCCESS, "ANF diagnostic adapter");

    for (uint32_t mode = 0; mode < 2; mode++) {
        AnfSRCreateInfo sr = {};
        sr.header.type = ANF_STYPE_SR_CREATE_INFO;
        sr.inputSize = {640, 360};
        sr.outputSize = {1280, 720};
        AnfTechniqueVulkanCreateInfo vk = {};
        vk.header.type = ANF_STYPE_TECHNIQUE_VULKAN_CREATE_INFO;
        AnfTechniqueCreateInfo techniqueCreate = {};
        techniqueCreate.header.type = ANF_STYPE_TECHNIQUE_CREATE_INFO;
        techniqueCreate.techniqueId = ANF_TECHNIQUE_ID_SR_TEMPORAL;
        techniqueCreate.flags = mode ? ANF_TECHNIQUE_CREATE_FLAG_DISPATCH_IMMEDIATE : 0;
        techniqueCreate.maxInFlight = 1;
        techniqueCreate.pClientApiCreateInfo = &vk.header;
        techniqueCreate.pTechniqueGroupCreateInfo = &sr.header;
        AnfTechnique technique = nullptr;
        AnfResult nativeResult = api.CreateTechnique(instance, &techniqueCreate, &technique);
        std::printf("ANF_NATIVE_CREATE_SR mode=%s result=%u native_validation=1\n", mode ? "immediate" : "recorded", unsigned(nativeResult));

        if (technique)
            api.DestroyTechnique(instance, technique);
    }

    api.DestroyInstance(instance);
    dlclose(library);

    UpscalerDesc upscaler = {};
    upscaler.type = UpscalerType::ANF;
    upscaler.mode = UpscalerMode::PERFORMANCE;
    upscaler.upscaleResolution = {1280, 720};
    upscaler.maxInFlight = 1;
    Result creation = fixture.upscaling.CreateUpscaler(*fixture.device, upscaler, fixture.upscaler);
    std::printf("ANF_CREATE_TECHNIQUE result=%u\n", unsigned(creation));

    if (creation == Result::UNSUPPORTED) {
        Require(fixture.upscaler == nullptr, "ANF rejected creation must return null");
        Require(fixture.errors == 0, "ANF rejected creation validation errors");
        std::printf("ANF_DRIVER_UNSUPPORTED creation=UNSUPPORTED output_null=1 dispatch_not_executed=1\n");

        return 77;
    }

    Require(creation == Result::SUCCESS, "ANF OpenRHI technique creation");

    for (uint32_t i = 0; i < 4; i++)
        fixture.CreateImage(i, requirements.resources[i]);

    const auto& output = fixture.images[1];
    Require(output.format == Format::RGBA8_UNORM || output.format == Format::RGBA16_SFLOAT, "ANF test output decoding");
    const uint32_t pixelStride = output.format == Format::RGBA8_UNORM ? 4 : 8;
    const uint64_t byteSize = uint64_t(output.width) * output.height * pixelStride;
    VkBufferCreateInfo buffer = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer.size = byteSize;
    buffer.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    Require(vkCreateBuffer(fixture.native, &buffer, nullptr, &fixture.readback) == VK_SUCCESS, "ANF readback buffer");
    VkMemoryRequirements memory = {};
    vkGetBufferMemoryRequirements(fixture.native, fixture.readback, &memory);
    VkMemoryAllocateInfo allocate = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = memory.size;
    allocate.memoryTypeIndex = fixture.MemoryType(memory.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Require(vkAllocateMemory(fixture.native, &allocate, nullptr, &fixture.readbackMemory) == VK_SUCCESS, "ANF readback allocation");
    Require(vkBindBufferMemory(fixture.native, fixture.readback, fixture.readbackMemory, 0) == VK_SUCCESS, "ANF readback bind");
    Require(fixture.core.CreateCommandAllocator(*fixture.queue, fixture.allocator) == Result::SUCCESS, "ANF allocator");
    Require(fixture.core.CreateCommandBuffer(*fixture.allocator, fixture.command) == Result::SUCCESS, "ANF command buffer");
    Require(fixture.core.CreateFence(*fixture.device, 0, fixture.fence) == Result::SUCCESS, "ANF fence");
    float previous[3] = {};

    for (uint32_t frame = 0; frame < 3; frame++) {
        if (frame)
            fixture.core.ResetCommandAllocator(*fixture.allocator);

        Require(fixture.core.BeginCommandBuffer(*fixture.command, nullptr) == Result::SUCCESS, "ANF begin");
        VkCommandBuffer command = (VkCommandBuffer)fixture.core.GetCommandBufferNativeObject(fixture.command);

        for (uint32_t i = 0; i < 4; i++) {
            fixture.Transition(i, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
            VkImageSubresourceRange range = {i == 3 ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            if (i == 3) {
                VkClearDepthStencilValue depth = {0.5f, 0};
                vkCmdClearDepthStencilImage(command, fixture.images[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &depth, 1, &range);
            } else {
                VkClearColorValue color = {};

                if (i == 0) {
                    color.float32[frame % 3] = 0.75f;
                    color.float32[(frame + 1) % 3] = 0.25f;
                    color.float32[3] = 1.0f;
                } else if (i == 1) {
                    color.float32[0] = color.float32[2] = color.float32[3] = 1.0f;
                }

                vkCmdClearColorImage(command, fixture.images[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);
            }

            fixture.Transition(i, i == 1 ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, i == 1 ? VK_ACCESS_2_SHADER_WRITE_BIT : VK_ACCESS_2_SHADER_READ_BIT);
        }

        DispatchUpscaleDesc dispatch = {};
        dispatch.input = {fixture.images[0].texture, fixture.images[0].view};
        dispatch.output = {fixture.images[1].texture, fixture.images[1].view};
        dispatch.guides.upscaler.mv = {fixture.images[2].texture, fixture.images[2].view};
        dispatch.guides.upscaler.depth = {fixture.images[3].texture, fixture.images[3].view};
        dispatch.currentResolution = {640, 360};
        dispatch.mvScale = {1.0f, 1.0f};
        dispatch.cameraJitter = {frame ? 0.25f : -0.25f, frame == 1 ? -0.25f : 0.25f};
        dispatch.flags = frame == 1 ? DispatchUpscaleBits::NONE : DispatchUpscaleBits::RESET_HISTORY;
        fixture.upscaling.CmdDispatchUpscale(*fixture.command, *fixture.upscaler, dispatch);
        fixture.Transition(1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
        VkBufferImageCopy copy = {};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {output.width, output.height, 1};
        vkCmdCopyImageToBuffer(command, output.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, fixture.readback, 1, &copy);
        VkMemoryBarrier2 host = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
        host.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        host.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        host.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
        host.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
        VkDependencyInfo dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency.memoryBarrierCount = 1;
        dependency.pMemoryBarriers = &host;
        vkCmdPipelineBarrier2(command, &dependency);
        Require(fixture.core.EndCommandBuffer(*fixture.command) == Result::SUCCESS, "ANF end");
        FenceSubmitDesc signal = {fixture.fence, frame + 1, StageBits::ALL};
        QueueSubmitDesc submit = {};
        submit.commandBuffers = &fixture.command;
        submit.commandBufferNum = 1;
        submit.signalFences = &signal;
        submit.signalFenceNum = 1;
        Require(fixture.core.QueueSubmit(*fixture.queue, submit) == Result::SUCCESS, "ANF OpenRHI submit");
        fixture.core.Wait(*fixture.fence, frame + 1);
        void* mapped = nullptr;
        Require(vkMapMemory(fixture.native, fixture.readbackMemory, 0, byteSize, 0, &mapped) == VK_SUCCESS, "ANF output map");
        float means[3] = {};
        uint32_t samples = 0;

        for (uint32_t y = 32; y < output.height - 32; y += 17) {
            for (uint32_t x = 32; x < output.width - 32; x += 17) {
                const uint64_t offset = uint64_t(y) * output.width + x;

                for (uint32_t channel = 0; channel < 3; channel++) {
                    float value = output.format == Format::RGBA8_UNORM ? ((uint8_t*)mapped)[offset * 4 + channel] / 255.0f : Half(((uint16_t*)mapped)[offset * 4 + channel]);
                    Require(std::isfinite(value), "ANF output contains non-finite pixels");
                    means[channel] += value;
                }

                samples++;
            }
        }

        vkUnmapMemory(fixture.native, fixture.readbackMemory);
        float difference = 0;

        for (uint32_t channel = 0; channel < 3; channel++) {
            means[channel] /= float(samples);
            difference += std::abs(means[channel] - previous[channel]);
            previous[channel] = means[channel];
        }

        std::printf("ANF_FRAME frame=%u reset=%d mean=%f,%f,%f changed=%f samples=%u validation_errors=%u\n", frame, frame != 1, means[0], means[1], means[2], difference, samples, fixture.errors);
        Require(means[frame % 3] > 0.4f && means[(frame + 2) % 3] < 0.3f, "ANF reconstructed color does not match input");
        Require(!frame || difference > 0.3f, "ANF output did not respond to input change");
        Require(fixture.errors == 0, "ANF validation errors");
    }

    std::printf("ANF_OPENRHI_SR_PASS frames=3 output=1280x720 validation_errors=%u\n", fixture.errors);

    return 0;
}
