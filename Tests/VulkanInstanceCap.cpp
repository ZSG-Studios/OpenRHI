#include <vulkan/vulkan.h>

#include <dlfcn.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

// Exercise a 1.3 loader with the real physical device and native instance/device functions
static PFN_vkGetInstanceProcAddr GetNativeLoader() {
#if defined(__ANDROID__)
    static void* library = dlopen("/system/lib64/libvulkan.so", RTLD_NOW | RTLD_LOCAL);
#else
    const char* path = std::getenv("OPENRHI_TEST_NATIVE_VULKAN_LOADER");
    static void* library = path ? dlopen(path, RTLD_NOW | RTLD_LOCAL) : nullptr;
#endif

    return library ? (PFN_vkGetInstanceProcAddr)dlsym(library, "vkGetInstanceProcAddr") : nullptr;
}

static VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceVersion(uint32_t* version) {
    const auto getProc = GetNativeLoader();
    const auto enumerate = getProc ? (PFN_vkEnumerateInstanceVersion)getProc(VK_NULL_HANDLE, "vkEnumerateInstanceVersion") : nullptr;

    if (!enumerate)
        return VK_ERROR_INITIALIZATION_FAILED;

    const VkResult result = enumerate(version);

    if (result == VK_SUCCESS)
        *version = std::min(*version, uint32_t(VK_API_VERSION_1_3));

    return result;
}

extern "C" VKAPI_ATTR __attribute__((visibility("default"))) PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* name) {
    if (std::strcmp(name, "vkEnumerateInstanceVersion") == 0)
        return (PFN_vkVoidFunction)EnumerateInstanceVersion;

    const auto getProc = GetNativeLoader();

    return getProc ? getProc(instance, name) : nullptr;
}
