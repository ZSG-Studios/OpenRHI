#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include "anf.h"
#include "anf_types_vk.h"

static void Log(AnfLogLevel level, const char* message) {
    std::printf("ANF[%u] %s\n", unsigned(level), message ? message : "");
}

static bool Has(const std::vector<VkExtensionProperties>& entries, const char* name) {
    for (const auto& entry : entries)
        if (!std::strcmp(entry.extensionName, name))
            return true;
    return false;
}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    void* library = dlopen(argc > 1 ? argv[1] : "./libanf.so", RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::printf("DLOPEN_ERROR %s\n", dlerror());
        return 1;
    }
    auto getFunctions = reinterpret_cast<void (*)(AnfFunctions*)>(dlsym(library, "GetAnfFunctions"));
    if (!getFunctions) {
        std::puts("MISSING_FUNCTION_TABLE");
        dlclose(library);
        return 1;
    }
    AnfFunctions anf = {};
    getFunctions(&anf);
    if (!anf.CreateInstance || !anf.DestroyInstance || !anf.QueryTechniqueRequirements || !anf.IsTechniqueSupported) {
        std::puts("INCOMPLETE_FUNCTION_TABLE");
        dlclose(library);
        return 1;
    }
    if (anf.QueryAnfVersion) {
        AnfVersion version = {};
        auto result = anf.QueryAnfVersion(&version);
        std::printf("ANF_VERSION result=%u version=%u.%u.%u\n", unsigned(result), version.major, version.minor, version.build);
    }
    AnfInstanceCreateInfo create = {};
    create.header.type = ANF_STYPE_INSTANCE_CREATE_INFO;
    create.clientApi = ANF_CLIENT_API_VULKAN;
    create.logLevel = ANF_LOG_LEVEL_INFO;
    create.logMessageCallback = Log;
    AnfInstance instance = {};
    auto result = anf.CreateInstance(&create, &instance);
    std::printf("ANF_CREATE_INSTANCE result=%u\n", unsigned(result));
    if (result != ANF_RESULT_SUCCESS) {
        dlclose(library);
        return 1;
    }
    bool error = false, supported = false;
    for (AnfTechniqueId technique : {ANF_TECHNIQUE_ID_SR_TEMPORAL, ANF_TECHNIQUE_ID_FG_TEMPORAL}) {
        const char* label = technique == ANF_TECHNIQUE_ID_SR_TEMPORAL ? "SR" : "FG";
        const AnfTechniqueRequirements* requirements = nullptr;
        result = anf.QueryTechniqueRequirements(instance, technique, &requirements);
        std::printf("%s REQUIREMENTS result=%u\n", label, unsigned(result));
        if (result != ANF_RESULT_SUCCESS) {
            error |= result != ANF_RESULT_NOT_SUPPORTED;
            continue;
        }
        if (!requirements || requirements->header.type != ANF_STYPE_TECHNIQUE_REQUIREMENTS || !requirements->pClientApiRequirements || requirements->pClientApiRequirements->type != ANF_STYPE_TECHNIQUE_VULKAN_API_REQUIREMENTS) {
            std::printf("%s INVALID_REQUIREMENTS\n", label);
            error = true;
            continue;
        }
        auto vkRequirements = reinterpret_cast<const AnfTechniqueVulkanApiRequirements*>(requirements->pClientApiRequirements);
        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> instanceExtensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, instanceExtensions.data());
        bool missing = false;
        for (uint32_t i = 0; i < vkRequirements->numInstanceExtensions; i++) {
            const char* extension = vkRequirements->ppInstanceExtensions[i];
            bool available = Has(instanceExtensions, extension);
            std::printf("%s INSTANCE_EXTENSION %s available=%d\n", label, extension, available);
            missing |= !available;
        }
        if (missing) {
            std::printf("%s UNSUPPORTED_MISSING_INSTANCE_EXTENSION\n", label);
            continue;
        }
        VkApplicationInfo application = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
        application.pApplicationName = "OpenRHI ANF compatibility probe";
        application.apiVersion = VK_API_VERSION_1_3;
        VkInstanceCreateInfo instanceCreate = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        instanceCreate.pApplicationInfo = &application;
        instanceCreate.enabledExtensionCount = vkRequirements->numInstanceExtensions;
        instanceCreate.ppEnabledExtensionNames = vkRequirements->ppInstanceExtensions;
        VkInstance vkInstance = VK_NULL_HANDLE;
        VkResult vkResult = vkCreateInstance(&instanceCreate, nullptr, &vkInstance);
        std::printf("%s VK_CREATE_INSTANCE result=%d\n", label, vkResult);
        if (vkResult != VK_SUCCESS) {
            error = true;
            continue;
        }
        count = 0;
        vkEnumeratePhysicalDevices(vkInstance, &count, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(count);
        vkEnumeratePhysicalDevices(vkInstance, &count, physicalDevices.data());
        for (VkPhysicalDevice physical : physicalDevices) {
            VkPhysicalDeviceProperties properties = {};
            vkGetPhysicalDeviceProperties(physical, &properties);
            std::printf("%s GPU=%s vendor=%u device=%u api=%u.%u.%u driver=%u\n", label, properties.deviceName, properties.vendorID, properties.deviceID, VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion), properties.driverVersion);
            count = 0;
            vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, nullptr);
            std::vector<VkExtensionProperties> deviceExtensions(count);
            vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, deviceExtensions.data());
            missing = false;
            for (uint32_t i = 0; i < vkRequirements->numDeviceExtensions; i++) {
                const char* extension = vkRequirements->ppDeviceExtensions[i];
                bool available = Has(deviceExtensions, extension);
                std::printf("%s DEVICE_EXTENSION %s available=%d\n", label, extension, available);
                missing |= !available;
            }
            if (missing) {
                std::printf("%s UNSUPPORTED_MISSING_DEVICE_EXTENSION\n", label);
                continue;
            }
            count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, nullptr);
            std::vector<VkQueueFamilyProperties> queues(count);
            vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, queues.data());
            uint32_t family = UINT32_MAX;
            for (uint32_t i = 0; i < count; i++)
                if (queues[i].queueCount && (queues[i].queueFlags & vkRequirements->techniqueQueueFlags) == vkRequirements->techniqueQueueFlags) {
                    family = i;
                    break;
                }
            if (family == UINT32_MAX) {
                std::printf("%s UNSUPPORTED_QUEUE_REQUIREMENTS\n", label);
                continue;
            }
            float priority = 1;
            VkDeviceQueueCreateInfo queue = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            queue.queueFamilyIndex = family;
            queue.queueCount = 1;
            queue.pQueuePriorities = &priority;
            VkPhysicalDeviceFeatures features = {};
            vkGetPhysicalDeviceFeatures(physical, &features);
            VkDeviceCreateInfo deviceCreate = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            deviceCreate.queueCreateInfoCount = 1;
            deviceCreate.pQueueCreateInfos = &queue;
            deviceCreate.enabledExtensionCount = vkRequirements->numDeviceExtensions;
            deviceCreate.ppEnabledExtensionNames = vkRequirements->ppDeviceExtensions;
            deviceCreate.pEnabledFeatures = &features;
            VkDevice device = VK_NULL_HANDLE;
            vkResult = vkCreateDevice(physical, &deviceCreate, nullptr, &device);
            std::printf("%s VK_CREATE_DEVICE result=%d\n", label, vkResult);
            if (vkResult != VK_SUCCESS) {
                error = true;
                continue;
            }
            AnfAdapterInfoVulkan adapter = {};
            adapter.header.type = ANF_STYPE_ADAPTER_INFO_VULKAN;
            adapter.instance = vkInstance;
            adapter.physicalDevice = physical;
            adapter.device = device;
            result = anf.IsTechniqueSupported(instance, technique, &adapter.header);
            std::printf("%s IS_TECHNIQUE_SUPPORTED result=%u supported=%d\n", label, unsigned(result), result == ANF_RESULT_SUCCESS);
            supported |= result == ANF_RESULT_SUCCESS;
            error |= result != ANF_RESULT_SUCCESS && result != ANF_RESULT_NOT_SUPPORTED;
            vkDestroyDevice(device, nullptr);
        }
        vkDestroyInstance(vkInstance, nullptr);
    }
    result = anf.DestroyInstance(instance);
    std::printf("ANF_DESTROY_INSTANCE result=%u\n", unsigned(result));
    dlclose(library);
    return error ? 1 : (supported ? 0 : 77);
}
