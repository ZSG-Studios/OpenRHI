// © 2021 NVIDIA Corporation

#ifdef _WIN32
#    include <windows.h> // OutputDebugStringA
#else
#    include <csignal> // raise
#    include <cstdlib> // malloc
#endif

#if OPENRHI_ENABLE_NVTX_SUPPORT
#    include "nvtx3/nvToolsExt.h"
#endif

#if OPENRHI_ENABLE_D3D12_SUPPORT
#    include <d3d12.h>
#    include <d3d12video.h>
#    include <dxgidebug.h>
#endif

#if OPENRHI_ENABLE_VK_SUPPORT
#    include <vulkan/vulkan.h>
#endif

#include "SharedExternal.h"

#define ADAPTER_MAX_NUM 32u

using namespace openrhi;

Result CreateDeviceNONE(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceD3D12(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationD3D12Desc& deviceCreationDescD3D12, DeviceBase*& device);
Result CreateDeviceVK(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationVKDesc& deviceCreationDescVK, DeviceBase*& device);
Result CreateDeviceMetal(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationMetalDesc& deviceCreationMetalDesc, DeviceBase*& device);
void UpdateAdaptersMetal(AdapterDesc* adapterDescs, uint32_t& adapterDescNum, uint32_t adapterDescMaxNum);
bool GetAdapterDescMetal(AdapterDesc& adapterDesc, void* device);
DeviceBase* CreateDeviceValidation(const DeviceCreationDesc& deviceCreationDesc, DeviceBase& device);

static constexpr uint64_t Hash(const char* name) {
    return *name != 0 ? *name ^ (33 * Hash(name + 1)) : 5381;
}

constexpr std::array<const char*, (size_t)Message::MAX_NUM> g_messageTypes = {
    "INFO",    // INFO,
    "WARNING", // WARNING,
    "ERROR",   // ERROR
};
OPENRHI_VALIDATE_ARRAY_BY_PTR(g_messageTypes);

static void OPENRHI_CALL MessageCallback(Message messageType, const char* file, uint32_t line, const char* message, void*) {
    const char* messageTypeName = g_messageTypes[(size_t)messageType];

    char buf[OPENRHI_MAX_MESSAGE_LENGTH];
    snprintf(buf, sizeof(buf), "%s (%s:%u) - %s\n", messageTypeName, file, line, message);

    fprintf(stderr, "%s", buf);
#ifdef _WIN32
    OutputDebugStringA(buf);
#endif
}

static void OPENRHI_CALL AbortExecution(void*) {
#ifdef _WIN32
    DebugBreak();
#else
    raise(SIGTRAP);
#endif
}

#ifdef _WIN32

static void* OPENRHI_CALL AlignedMalloc(void*, size_t size, size_t alignment) {
    return _aligned_malloc(size, alignment);
}

static void* OPENRHI_CALL AlignedRealloc(void*, void* memory, size_t size, size_t alignment) {
    return _aligned_realloc(memory, size, alignment);
}

static void OPENRHI_CALL AlignedFree(void*, void* memory) {
    _aligned_free(memory);
}

#else

// FIXED BY AI: Preserve payload contents when reallocation changes the aligned offset within the raw allocation.
struct AllocationHeader {
    void* memory;
    size_t size;
};

static void* OPENRHI_CALL AlignedMalloc(void*, size_t size, size_t alignment) {
    const size_t effectiveAlignment = std::max(alignment, alignof(AllocationHeader));

    if ((effectiveAlignment & (effectiveAlignment - 1)) || effectiveAlignment > std::numeric_limits<size_t>::max() - sizeof(AllocationHeader) + 1)
        return nullptr;

    const size_t overhead = sizeof(AllocationHeader) + effectiveAlignment - 1;

    if (size > std::numeric_limits<size_t>::max() - overhead)
        return nullptr;

    uint8_t* memory = (uint8_t*)malloc(size + overhead);

    if (!memory)
        return nullptr;

    uint8_t* alignedMemory = Align(memory + sizeof(AllocationHeader), effectiveAlignment);
    AllocationHeader* header = (AllocationHeader*)alignedMemory - 1;
    header->memory = memory;
    header->size = size;

    return alignedMemory;
}

static void* OPENRHI_CALL AlignedRealloc(void* userArg, void* memory, size_t size, size_t alignment) {
    if (!memory)
        return AlignedMalloc(userArg, size, alignment);

    AllocationHeader* oldHeader = (AllocationHeader*)memory - 1;
    void* newMemory = AlignedMalloc(userArg, size, alignment);

    if (!newMemory)
        return nullptr;

    memcpy(newMemory, memory, std::min(size, oldHeader->size));
    free(oldHeader->memory);

    return newMemory;
}

static void OPENRHI_CALL AlignedFree(void*, void* memory) {
    if (!memory)
        return;

    free(((AllocationHeader*)memory - 1)->memory);
}

#endif

static void CheckAndSetDefaultCallbacks(DeviceCreationDesc& deviceCreationDesc) {
    if (!deviceCreationDesc.callbackInterface.MessageCallback)
        deviceCreationDesc.callbackInterface.MessageCallback = MessageCallback;

    if (!deviceCreationDesc.callbackInterface.AbortExecution)
        deviceCreationDesc.callbackInterface.AbortExecution = AbortExecution;

    if (!deviceCreationDesc.allocationCallbacks.Allocate || !deviceCreationDesc.allocationCallbacks.Reallocate || !deviceCreationDesc.allocationCallbacks.Free) {
        deviceCreationDesc.allocationCallbacks.Allocate = AlignedMalloc;
        deviceCreationDesc.allocationCallbacks.Reallocate = AlignedRealloc;
        deviceCreationDesc.allocationCallbacks.Free = AlignedFree;
    }
}

static int SortAdapters(const void* pa, const void* pb) {
    const AdapterDesc& a = *(const AdapterDesc*)pa;
    const AdapterDesc& b = *(const AdapterDesc*)pb;
    return IsAdapterPreferred(a, b) ? -1 : (IsAdapterPreferred(b, a) ? 1 : 0);
}

#if OPENRHI_ENABLE_D3D12_SUPPORT

static void UpdateAdaptersD3D(AdapterDesc* adapterDescs, uint32_t& adapterDescNum, LUID* precreatedLuid) {
    ComPtr<IDXGIFactory4> dxgifactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgifactory))))
        return;

    for (uint32_t i = 0; i < 256 && adapterDescNum < ADAPTER_MAX_NUM; i++) {
        ComPtr<IDXGIAdapter1> adapter;
        HRESULT hr = dxgifactory->EnumAdapters1(i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc = {};
        if (FAILED(adapter->GetDesc1(&desc)))
            continue;

        // Logic: append unique or wait for "precreated"
        Uid_t uid = {};
        memcpy(&uid.low, &desc.AdapterLuid, sizeof(uid.low));

        uint32_t n = 0;
        if (precreatedLuid) {
            Uid_t uidNeeded = {};
            memcpy(&uidNeeded.low, precreatedLuid, sizeof(uidNeeded.low));

            if (!CompareUid(uid, uidNeeded))
                continue;
        } else {
            for (; n < adapterDescNum; n++) {
                if (CompareUid(adapterDescs[n].uid, uid))
                    break;
            }
        }

        AdapterDesc& adapterDesc = adapterDescs[n];

        // Update GAPI support
        ComPtr<ID3D12Device> enumerationDevice;
        hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&enumerationDevice));
        if (SUCCEEDED(hr))
            adapterDesc.supportedGraphicsAPIs |= GraphicsAPI::D3D12;

        // Logic: advance or skip
        if (n != adapterDescNum)
            continue;

        adapterDescNum++;

        { // Driver version
            LARGE_INTEGER driverVersion = {};
            if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driverVersion)))
                adapterDesc.driverVersion = driverVersion.LowPart;
            adapterDesc.nativeDriverVersion = uint64_t(driverVersion.QuadPart);
        }

        // Architecture
        if (desc.Flags == DXGI_ADAPTER_FLAG_NONE) {
            if (enumerationDevice) {
                D3D12_FEATURE_DATA_ARCHITECTURE architecture = {};
                hr = enumerationDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &architecture, sizeof(architecture));
                if (SUCCEEDED(hr))
                    adapterDesc.architecture = architecture.UMA ? Architecture::INTEGRATED : Architecture::DISCRETE;
            }
        } else if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            adapterDesc.architecture = Architecture::SOFTWARE;
        else if (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE)
            adapterDesc.architecture = Architecture::VIRTUAL;

        // Advertise reasonable values for queue counts (can't be queried in D3D)
        adapterDesc.queueNum[(uint32_t)QueueType::GRAPHICS] = 4;
        adapterDesc.queueNum[(uint32_t)QueueType::COMPUTE] = 4;
        adapterDesc.queueNum[(uint32_t)QueueType::COPY] = 4;

#    if OPENRHI_ENABLE_D3D12_SUPPORT
        if (enumerationDevice) {
            ComPtr<ID3D12VideoDevice> videoDevice;
            HRESULT hrD3D12 = enumerationDevice->QueryInterface(IID_PPV_ARGS(&videoDevice));
            if (SUCCEEDED(hrD3D12)) {
                adapterDesc.queueNum[(uint32_t)QueueType::VIDEO_DECODE] = 4;
                adapterDesc.queueNum[(uint32_t)QueueType::VIDEO_ENCODE] = 4;
            }
        }
#    endif

        // Other fields
        adapterDesc.uid = uid;
        adapterDesc.vendorId = desc.VendorId;
        adapterDesc.revisionId = desc.Revision;
        adapterDesc.revisionIdKnown = true;
        adapterDesc.deviceId = desc.DeviceId;
        adapterDesc.vendor = GetVendorFromID(desc.VendorId);
        adapterDesc.videoMemorySize = desc.DedicatedVideoMemory + desc.DedicatedSystemMemory;
        adapterDesc.sharedSystemMemorySize = desc.SharedSystemMemory;
        VideoMemoryInfo budget = {};
        if (QueryVideoMemoryInfoDXGI(adapterDesc.uid.low, MemoryLocation::DEVICE, budget) == Result::SUCCESS) {
            adapterDesc.memoryBudget = budget.budgetSize > budget.usageSize ? budget.budgetSize - budget.usageSize : 0;
            adapterDesc.memoryBudgetKnown = true;
        }

        wcstombs(adapterDesc.name, desc.Description, GetCountOf(adapterDesc.name) - 1);

        // Logic: only one "adapterDesc" is needed for "precreated"
        if (precreatedLuid)
            break;
    }
}

#endif

#if OPENRHI_ENABLE_VK_SUPPORT

static uint32_t GetVideoCodecNumVK(VkVideoCodecOperationFlagsKHR videoCodecOperations, bool decode) {
    constexpr VkVideoCodecOperationFlagsKHR VIDEO_DECODE_CODEC_OPERATION_MASK_CREATION = 0x0000FFFF;
    constexpr VkVideoCodecOperationFlagsKHR VIDEO_ENCODE_CODEC_OPERATION_MASK_CREATION = 0xFFFF0000;

    const VkVideoCodecOperationFlagsKHR mask = decode ? VIDEO_DECODE_CODEC_OPERATION_MASK_CREATION : VIDEO_ENCODE_CODEC_OPERATION_MASK_CREATION;
    videoCodecOperations &= mask;

    uint32_t num = 0;
    while (videoCodecOperations) {
        num += videoCodecOperations & 1;
        videoCodecOperations >>= 1;
    }

    return num;
}

static void UpdateAdaptersVK(AdapterDesc* adapterDescs, uint32_t& adapterDescNum, VkPhysicalDevice precreatedPhysicalDevice) {
    // Variables first
    VkApplicationInfo applicationInfo = {};
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    VkInstance instance = VK_NULL_HANDLE;
    Uid_t uidNeeded = {};
    VkResult vkResult = VK_SUCCESS;
    uint32_t deviceGroupNum = 0;
    uint32_t maxFamilyNum = 1;
    VkPhysicalDeviceGroupProperties* deviceGroupProperties = nullptr;
    VkQueueFamilyProperties2* familyProps2 = nullptr;
    VkQueueFamilyVideoPropertiesKHR* familyVideoProps = nullptr;

    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    PFN_vkCreateInstance vkCreateInstance = nullptr;
    PFN_vkDestroyInstance vkDestroyInstance = nullptr;
    PFN_vkEnumeratePhysicalDeviceGroups vkEnumeratePhysicalDeviceGroups = nullptr;
    PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2 = nullptr;
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2 = nullptr;

    // Get loader
    Library* loader = LoadSharedLibrary(OPENRHI_VULKAN_LOADER_NAME);
    if (!loader)
        goto CLEANUP;

    // Get the entry point
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetSharedLibraryFunction(*loader, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr)
        goto CLEANUP;

    // Create instance
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    if (!vkCreateInstance)
        goto CLEANUP;

    vkResult = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if (vkResult != VK_SUCCESS)
        goto CLEANUP;

    // Get needed functions
    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    vkEnumeratePhysicalDeviceGroups = (PFN_vkEnumeratePhysicalDeviceGroups)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDeviceGroups");
    vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2");
    vkGetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties2");
    vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
    vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
    vkGetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2");

    if (!vkDestroyInstance || !vkEnumeratePhysicalDeviceGroups || !vkGetPhysicalDeviceProperties2)
        goto CLEANUP;

    vkResult = vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, nullptr);
    if (vkResult != VK_SUCCESS)
        goto CLEANUP;

    // Query device groups
    deviceGroupProperties = (VkPhysicalDeviceGroupProperties*)alloca(sizeof(VkPhysicalDeviceGroupProperties) * deviceGroupNum);
    for (uint32_t i = 0; i < deviceGroupNum; i++) {
        deviceGroupProperties[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
        deviceGroupProperties[i].pNext = nullptr;
    }
    vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, deviceGroupProperties);

    // Max queue families
    if (vkGetPhysicalDeviceQueueFamilyProperties2) {
        for (uint32_t i = 0; i < deviceGroupNum; i++) {
            VkPhysicalDevice physicalDevice = deviceGroupProperties[i].physicalDevices[0];

            uint32_t familyNum = 0;
            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &familyNum, nullptr);

            maxFamilyNum = std::max(maxFamilyNum, familyNum);
        }
    }

    familyProps2 = (VkQueueFamilyProperties2*)alloca(sizeof(VkQueueFamilyProperties2) * maxFamilyNum);
    familyVideoProps = (VkQueueFamilyVideoPropertiesKHR*)alloca(sizeof(VkQueueFamilyVideoPropertiesKHR) * maxFamilyNum);
    for (uint32_t i = 0; i < maxFamilyNum; i++) {
        familyProps2[i] = {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2};
        familyVideoProps[i] = {VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR};
        familyProps2[i].pNext = &familyVideoProps[i];
    }

    // Precreated physical device
    if (precreatedPhysicalDevice) {
        VkPhysicalDeviceProperties2 deviceProps2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

        VkPhysicalDeviceIDProperties deviceIDProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
        VkPhysicalDeviceDriverProperties driverProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
        deviceIDProperties.pNext = &driverProperties;
        deviceProps2.pNext = &deviceIDProperties;

        vkGetPhysicalDeviceProperties2(precreatedPhysicalDevice, &deviceProps2);

        uidNeeded = ConstructUid(deviceIDProperties.deviceLUID, deviceIDProperties.deviceUUID, deviceIDProperties.deviceLUIDValid);
    }

    // Query device groups properties
    for (uint32_t i = 0; i < deviceGroupNum && adapterDescNum < ADAPTER_MAX_NUM; i++) {
        VkPhysicalDevice physicalDevice = deviceGroupProperties[i].physicalDevices[0];

        VkPhysicalDeviceProperties2 deviceProps2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

        VkPhysicalDeviceIDProperties deviceIDProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
        VkPhysicalDeviceDriverProperties driverProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
        deviceIDProperties.pNext = &driverProperties;
        deviceProps2.pNext = &deviceIDProperties;

        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);

        // Logic: append unique or wait for "precreated"
        Uid_t uid = ConstructUid(deviceIDProperties.deviceLUID, deviceIDProperties.deviceUUID, deviceIDProperties.deviceLUIDValid);

        uint32_t n = 0;
        if (precreatedPhysicalDevice) {
            if (!CompareUid(uid, uidNeeded))
                continue;
        } else {
            for (; n < adapterDescNum; n++) {
                if (CompareUid(adapterDescs[n].uid, uid))
                    break;
            }
        }

        AdapterDesc& adapterDesc = adapterDescs[n];

        // Update GAPI support
        adapterDesc.supportedGraphicsAPIs |= GraphicsAPI::VK;

        // Logic: advance or skip
        if (n != adapterDescNum)
            continue;

        adapterDescNum++;

        // Architecture
        const VkPhysicalDeviceProperties& deviceProps = deviceProps2.properties;

        if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            adapterDesc.architecture = Architecture::DISCRETE;
        else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            adapterDesc.architecture = Architecture::INTEGRATED;
        else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
            adapterDesc.architecture = Architecture::SOFTWARE;
        else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
            adapterDesc.architecture = Architecture::VIRTUAL;

        // Memory size
        if (vkGetPhysicalDeviceMemoryProperties) {
            VkPhysicalDeviceMemoryProperties memoryProperties = {};
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
            for (uint32_t j = 0; j < memoryProperties.memoryHeapCount; j++) {
                // From spec: In UMA systems ... implementation must advertise the heap as device-local
                if ((memoryProperties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0 && deviceProps.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                    adapterDesc.videoMemorySize += memoryProperties.memoryHeaps[j].size;
                else
                    adapterDesc.sharedSystemMemorySize += memoryProperties.memoryHeaps[j].size;
            }
        }

        // Queues
        if (vkGetPhysicalDeviceQueueFamilyProperties2) {
            uint32_t familyNum = maxFamilyNum;
            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &familyNum, familyProps2);

            std::array<uint32_t, (size_t)QueueType::MAX_NUM> scores = {};
            for (uint32_t j = 0; j < familyNum; j++) {
                const VkQueueFamilyProperties& familyProps = familyProps2[j].queueFamilyProperties;
                const VkVideoCodecOperationFlagsKHR videoCodecOperations = familyVideoProps[j].videoCodecOperations;

                QueueFamilyProps props = {};
                props.queueCount = familyProps.queueCount;
                props.videoDecodeCodecNum = GetVideoCodecNumVK(videoCodecOperations, true);
                props.videoEncodeCodecNum = GetVideoCodecNumVK(videoCodecOperations, false);
                props.graphics = familyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT;
                props.compute = familyProps.queueFlags & VK_QUEUE_COMPUTE_BIT;
                props.copy = familyProps.queueFlags & VK_QUEUE_TRANSFER_BIT;
                props.sparse = familyProps.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
                props.videoDecode = (familyProps.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) && props.videoDecodeCodecNum;
                props.videoEncode = (familyProps.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) && props.videoEncodeCodecNum;
                props.protect = familyProps.queueFlags & VK_QUEUE_PROTECTED_BIT;
                props.opticalFlow = familyProps.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV;

                QueueType queueType = TrySelectPreferredQueueType(props, scores);
                while (queueType != QueueType::MAX_NUM) {
                    adapterDesc.queueNum[(size_t)queueType] = familyProps.queueCount;
                    queueType = TrySelectPreferredQueueType(props, scores);
                }
            }
        }

        // Other fields
        adapterDesc.uid = uid;
        adapterDesc.vendorId = deviceProps.vendorID;
        adapterDesc.deviceId = deviceProps.deviceID;
        adapterDesc.nativeDriverVersion = deviceProps.driverVersion;
        adapterDesc.nativeDriverId = uint32_t(driverProperties.driverID);
        memcpy(adapterDesc.deviceUuid, deviceIDProperties.deviceUUID, 16);
        memcpy(adapterDesc.driverUuid, deviceIDProperties.driverUUID, 16);
        snprintf(adapterDesc.driverDescription, sizeof(adapterDesc.driverDescription), "%.120s %.120s", driverProperties.driverName, driverProperties.driverInfo);
        adapterDesc.memoryBudget = 0;
        adapterDesc.memoryBudgetKnown = false;
        if (vkEnumerateDeviceExtensionProperties && vkGetPhysicalDeviceMemoryProperties2) {
            uint32_t extensionCount = 0;
            if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr) == VK_SUCCESS) {
                std::vector<VkExtensionProperties> extensions(extensionCount);
                if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data()) == VK_SUCCESS) {
                    for (const VkExtensionProperties& extension : extensions) {
                        if (strcmp(extension.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) != 0) continue;
                        VkPhysicalDeviceMemoryBudgetPropertiesEXT budget = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};
                        VkPhysicalDeviceMemoryProperties2 memory = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, &budget};
                        vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &memory);
                        adapterDesc.memoryBudgetKnown = true;
                        for (uint32_t heap = 0; heap < memory.memoryProperties.memoryHeapCount; heap++)
                            if (memory.memoryProperties.memoryHeaps[heap].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                                adapterDesc.memoryBudget += budget.heapBudget[heap] > budget.heapUsage[heap] ? budget.heapBudget[heap] - budget.heapUsage[heap] : 0;
                        break;
                    }
                }
            }
        }
        adapterDesc.driverVersion = deviceProps.driverVersion;
        adapterDesc.vendor = GetVendorFromID(deviceProps.vendorID);

        strncpy(adapterDesc.name, deviceProps.deviceName, sizeof(adapterDesc.name));

        // Logic: only one "adapterDesc" is needed for "precreated"
        if (precreatedPhysicalDevice)
            break;
    }

CLEANUP:
    if (vkDestroyInstance && instance)
        vkDestroyInstance(instance, nullptr);

    if (loader)
        UnloadSharedLibrary(*loader);
}

#endif

static Result FinalizeDeviceCreation(const DeviceCreationDesc& deviceCreationDesc, DeviceBase& deviceImpl, Device*& device) {
    Result compatible = CheckDeviceRequirements(deviceCreationDesc, deviceImpl.GetDesc());
    if (compatible != Result::SUCCESS) {
        deviceImpl.Destruct();
        device = nullptr;
        return compatible;
    }
    deviceImpl.InitializeDeviceProfile(ResolveDeviceProfile(deviceImpl.GetDesc(), deviceCreationDesc.profilePolicy));
#if OPENRHI_ENABLE_VALIDATION_SUPPORT
    if (deviceCreationDesc.enableOpenRHIValidation && deviceCreationDesc.graphicsAPI != GraphicsAPI::NONE) {
        Device* deviceVal = (Device*)CreateDeviceValidation(deviceCreationDesc, deviceImpl);
        if (!deviceVal) {
            openrhiDestroyDevice((Device*)&deviceImpl);
            device = nullptr;
            if (deviceCreationDesc.compatibilityReport)
                deviceCreationDesc.compatibilityReport->result = Result::FAILURE;
            return Result::FAILURE;
        }

        ((DeviceBase*)deviceVal)->InitializeDeviceProfile(deviceImpl.GetDeviceProfile());
        device = deviceVal;
    } else
#endif
        device = (Device*)&deviceImpl;

#if OPENRHI_ENABLE_NVTX_SUPPORT
    nvtxInitialize(nullptr); // needed only to avoid stalls on the first use
#endif

    return Result::SUCCESS;
}

OPENRHI_API Result OPENRHI_CALL openrhiGetInterface(const Device& device, const char* interfaceName, size_t interfaceSize, void* interfacePtr) {
    if (strstr(interfaceName, "openrhi::") == interfaceName)
        interfaceName += sizeof("openrhi::") - 1;
    else if (strstr(interfaceName, "OpenRhi") == interfaceName)
        interfaceName += sizeof("OpenRhi") - 1;

    uint64_t hash = Hash(interfaceName);
    size_t realInterfaceSize = size_t(-1);
    Result result = Result::INVALID_ARGUMENT;
    const DeviceBase& deviceBase = (DeviceBase&)device;

    memset(interfacePtr, 0, interfaceSize);

    if (hash == Hash(OPENRHI_STRINGIFY(CoreInterface))) {
        realInterfaceSize = sizeof(CoreInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(CoreInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(DescriptorHeapInterface))) {
        realInterfaceSize = sizeof(DescriptorHeapInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(DescriptorHeapInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(ImguiInterface))) {
        realInterfaceSize = sizeof(ImguiInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(ImguiInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(HelperInterface))) {
        realInterfaceSize = sizeof(HelperInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(HelperInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(LowLatencyInterface))) {
        realInterfaceSize = sizeof(LowLatencyInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(LowLatencyInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(MeshShaderInterface))) {
        realInterfaceSize = sizeof(MeshShaderInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(MeshShaderInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(RayTracingInterface))) {
        realInterfaceSize = sizeof(RayTracingInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(RayTracingInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(VideoInterface))) {
        realInterfaceSize = sizeof(VideoInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(VideoInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(StreamerInterface))) {
        realInterfaceSize = sizeof(StreamerInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(StreamerInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(SwapChainInterface))) {
        realInterfaceSize = sizeof(SwapChainInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(SwapChainInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(UpscalerInterface))) {
        realInterfaceSize = sizeof(UpscalerInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(UpscalerInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(WrapperD3D12Interface))) {
        realInterfaceSize = sizeof(WrapperD3D12Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D12Interface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(WrapperMetalInterface))) {
        realInterfaceSize = sizeof(WrapperMetalInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperMetalInterface*)interfacePtr);
    } else if (hash == Hash(OPENRHI_STRINGIFY(WrapperVKInterface))) {
        realInterfaceSize = sizeof(WrapperVKInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperVKInterface*)interfacePtr);
    }

    if (result == Result::INVALID_ARGUMENT)
        OPENRHI_REPORT_ERROR(&deviceBase, "Unknown interface '%s'!", interfaceName);
    else if (interfaceSize != realInterfaceSize)
        OPENRHI_REPORT_ERROR(&deviceBase, "Interface '%s' has invalid size=%u bytes, while %u bytes expected by the implementation", interfaceName, interfaceSize, realInterfaceSize);
    else if (result == Result::UNSUPPORTED)
        OPENRHI_REPORT_WARNING(&deviceBase, "Interface '%s' is not supported by the device!", interfaceName);
    else {
        const void* const* const begin = (void**)interfacePtr;
        const void* const* const end = begin + realInterfaceSize / sizeof(void*);

        for (const void* const* current = begin; current != end; current++) {
            if (!(*current)) {
                OPENRHI_REPORT_ERROR(&deviceBase, "Invalid function table: function #%u is NULL!", uint32_t(current - begin));
                return Result::FAILURE;
            }
        }
    }

    return result;
}

OPENRHI_API void OPENRHI_CALL openrhiBeginAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(name, bgra);

#if OPENRHI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if OPENRHI_ENABLE_NVTX_SUPPORT

    nvtxEventAttributes_t eventAttrib = {};
    eventAttrib.version = NVTX_VERSION;
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType = bgra == BGRA_UNUSED ? NVTX_COLOR_UNKNOWN : NVTX_COLOR_ARGB;
    eventAttrib.color = bgra;
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;

    nvtxRangePushEx(&eventAttrib);

#    else

    // TODO: add PIX

#    endif
#endif
}

OPENRHI_API void OPENRHI_CALL openrhiEndAnnotation() {
#if OPENRHI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if OPENRHI_ENABLE_NVTX_SUPPORT

    nvtxRangePop();

#    else

    // TODO: add PIX

#    endif
#endif
}

OPENRHI_API void OPENRHI_CALL openrhiAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(name, bgra);

#if OPENRHI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if OPENRHI_ENABLE_NVTX_SUPPORT

    nvtxEventAttributes_t eventAttrib = {};
    eventAttrib.version = NVTX_VERSION;
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType = bgra == BGRA_UNUSED ? NVTX_COLOR_UNKNOWN : NVTX_COLOR_ARGB;
    eventAttrib.color = bgra;
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;

    nvtxMarkEx(&eventAttrib);

#    else

    // TODO: add PIX

#    endif
#endif
}

#if OPENRHI_ENABLE_NVTX_SUPPORT
#    if (defined __linux__)
#        include <sys/syscall.h>
#    elif (defined __APPLE__)
#        include <sys/syscall.h>
#    elif (defined __ANDROID__)
#        include <unistd.h>
#    elif (defined _WIN64)
#        include <processthreadsapi.h>
#    endif

OPENRHI_API void OPENRHI_CALL openrhiSetThreadName(const char* name) {
    uint64_t tid = 0;
#    if (defined __linux__)
    tid = syscall(SYS_gettid);
#    elif (defined __APPLE__)
    pthread_threadid_np(nullptr, &tid);
#    elif (defined __ANDROID__)
    tid = gettid();
#    elif (defined _WIN64)
    tid = GetCurrentThreadId();
#    endif

    nvtxNameOsThreadA((uint32_t)tid, name);
}

#else

OPENRHI_API void OPENRHI_CALL openrhiSetThreadName(const char*) {
}

#endif

static bool IsBackendAvailable(GraphicsAPI api) {
    switch (api) {
#if OPENRHI_ENABLE_D3D12_SUPPORT
        case GraphicsAPI::D3D12: {
            using QueryVersion = LONG (WINAPI*)(OSVERSIONINFOW*);
            auto query = (QueryVersion)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
            OSVERSIONINFOW version = {};
            version.dwOSVersionInfoSize = sizeof(version);
            return query && query(&version) == 0 && version.dwMajorVersion >= 10 && version.dwBuildNumber >= 22000;
        }
#endif
#if OPENRHI_ENABLE_VK_SUPPORT
        case GraphicsAPI::VK: return true;
#endif
#if OPENRHI_ENABLE_METAL_SUPPORT
        case GraphicsAPI::METAL: return true;
#endif
#if OPENRHI_ENABLE_NONE_SUPPORT
        case GraphicsAPI::NONE: return true;
#endif
        default: return false;
    }
}

static Result ReportCreationFailure(const DeviceCreationDesc& desc, Result result) {
    if (desc.compatibilityReport) desc.compatibilityReport->result = result;
    return result;
}

OPENRHI_API Result OPENRHI_CALL openrhiCreateDevice(const DeviceCreationDesc& deviceCreationDesc, Device*& device) {
    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

    DeviceCreationDesc modifiedDeviceCreationDesc = deviceCreationDesc;
    CheckAndSetDefaultCallbacks(modifiedDeviceCreationDesc);

    device = nullptr;
    InitializeCompatibilityReport(deviceCreationDesc);
    if (!IsBackendAvailable(deviceCreationDesc.graphicsAPI))
        return ReportCreationFailure(deviceCreationDesc, Result::UNSUPPORTED);
    if (deviceCreationDesc.compatibilityReport)
        deviceCreationDesc.compatibilityReport->backendAvailable = true;
    if (uint32_t(deviceCreationDesc.profilePolicy) >= uint32_t(ProfilePolicy::MAX_NUM))
        return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

    // Evaluate compatible candidates in deterministic preference order
    uint32_t adapterDescNum = ADAPTER_MAX_NUM;
    std::array<AdapterDesc, ADAPTER_MAX_NUM> adapterDescs = {};
    AdapterDesc adapterDesc = {};
    if (!modifiedDeviceCreationDesc.adapterDesc) {
        if (modifiedDeviceCreationDesc.graphicsAPI != GraphicsAPI::NONE) {
            Result enumeration = openrhiEnumerateAdapters(adapterDescs.data(), adapterDescNum);
            if (enumeration != Result::SUCCESS) return ReportCreationFailure(deviceCreationDesc, enumeration);
            DeviceCompatibilityReport firstFailure = {};
            bool failedCandidate = false;
            for (uint32_t i = 0; i < adapterDescNum; i++) {
                if (adapterDescs[i].architecture == Architecture::SOFTWARE)
                    continue;
                if (!(adapterDescs[i].supportedGraphicsAPIs & modifiedDeviceCreationDesc.graphicsAPI))
                    continue;
                DeviceCompatibilityReport candidateReport = {};
                DeviceCreationDesc attempt = modifiedDeviceCreationDesc;
                attempt.adapterDesc = &adapterDescs[i];
                attempt.compatibilityReport = &candidateReport;
                result = openrhiCreateDevice(attempt, device);
                candidateReport.result = result;
                if (result == Result::SUCCESS) {
                    if (modifiedDeviceCreationDesc.compatibilityReport)
                        *modifiedDeviceCreationDesc.compatibilityReport = candidateReport;
                    return result;
                }
                if (!failedCandidate) {
                    firstFailure = candidateReport;
                    failedCandidate = true;
                }
            }
            if (failedCandidate && modifiedDeviceCreationDesc.compatibilityReport)
                *modifiedDeviceCreationDesc.compatibilityReport = firstFailure;
            return failedCandidate ? firstFailure.result : Result::UNSUPPORTED;
        }
        adapterDesc.supportedGraphicsAPIs = GraphicsAPI::NONE;
        for (uint32_t i = 0; i < uint32_t(QueueType::MAX_NUM); i++)
            adapterDesc.queueNum[i] = 1;
        modifiedDeviceCreationDesc.adapterDesc = &adapterDesc;
    }

    if ((modifiedDeviceCreationDesc.adapterDesc->supportedGraphicsAPIs & modifiedDeviceCreationDesc.graphicsAPI) == 0)
        return ReportCreationFailure(deviceCreationDesc, Result::UNSUPPORTED);

    if (modifiedDeviceCreationDesc.queueFamilyNum && !modifiedDeviceCreationDesc.queueFamilies)
        return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

    // Valid queue families expected
    Vector<QueueFamilyDesc> queueFamilies(modifiedDeviceCreationDesc.allocationCallbacks);

    if (!modifiedDeviceCreationDesc.queueFamilyNum) {
        QueueFamilyDesc graphicsQueue = {};
        graphicsQueue.queueNum = 1;
        graphicsQueue.queueType = QueueType::GRAPHICS;
        queueFamilies.push_back(graphicsQueue);

        modifiedDeviceCreationDesc.queueFamilyNum = 1;
    } else
        queueFamilies.assign(modifiedDeviceCreationDesc.queueFamilies, modifiedDeviceCreationDesc.queueFamilies + modifiedDeviceCreationDesc.queueFamilyNum);

    modifiedDeviceCreationDesc.queueFamilies = queueFamilies.data();

    for (uint32_t i = 0; i < modifiedDeviceCreationDesc.queueFamilyNum; i++) {
        QueueFamilyDesc& queueFamily = queueFamilies[i];

        uint32_t queueType = (uint32_t)queueFamily.queueType;
        if (queueType >= (uint32_t)QueueType::MAX_NUM)
            return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

        uint32_t supportedQueueNum = modifiedDeviceCreationDesc.adapterDesc->queueNum[queueType];
        if (queueFamily.queueNum > supportedQueueNum)
            queueFamily.queueNum = supportedQueueNum;
    }

#if OPENRHI_ENABLE_NONE_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::NONE)
        result = CreateDeviceNONE(modifiedDeviceCreationDesc, deviceImpl);
#endif

#if OPENRHI_ENABLE_D3D12_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D12)
        result = CreateDeviceD3D12(modifiedDeviceCreationDesc, {}, deviceImpl);
#endif

#if OPENRHI_ENABLE_VK_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::VK)
        result = CreateDeviceVK(modifiedDeviceCreationDesc, {}, deviceImpl);
#endif

#if OPENRHI_ENABLE_METAL_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::METAL)
        result = CreateDeviceMetal(modifiedDeviceCreationDesc, {}, deviceImpl);
#endif

    if (result != Result::SUCCESS) {
        if (deviceCreationDesc.compatibilityReport)
            deviceCreationDesc.compatibilityReport->result = result;
        device = nullptr;
        return result;
    }

    return FinalizeDeviceCreation(modifiedDeviceCreationDesc, *deviceImpl, device);
}

#if OPENRHI_ENABLE_METAL_SUPPORT
OPENRHI_API Result OPENRHI_CALL openrhiCreateDeviceFromMetalDevice(const DeviceCreationMetalDesc& deviceCreationMetalDesc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    device = nullptr;
    deviceCreationDesc.requirements = deviceCreationMetalDesc.requirements;
    deviceCreationDesc.compatibilityReport = deviceCreationMetalDesc.compatibilityReport;
    deviceCreationDesc.profilePolicy = deviceCreationMetalDesc.profilePolicy;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::METAL;

    AdapterDesc adapterDesc = {};
    deviceCreationDesc.adapterDesc = &adapterDesc;

    // Copy what is possible to the main "desc"
    deviceCreationDesc.callbackInterface = deviceCreationMetalDesc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationMetalDesc.allocationCallbacks;
    deviceCreationDesc.enableOpenRHIValidation = deviceCreationMetalDesc.enableOpenRHIValidation;
    deviceCreationDesc.enableMemoryZeroInitialization = deviceCreationMetalDesc.enableMemoryZeroInitialization;

    CheckAndSetDefaultCallbacks(deviceCreationDesc);
    InitializeCompatibilityReport(deviceCreationDesc);
    if (!IsBackendAvailable(deviceCreationDesc.graphicsAPI))
        return ReportCreationFailure(deviceCreationDesc, Result::UNSUPPORTED);
    if (deviceCreationDesc.compatibilityReport)
        deviceCreationDesc.compatibilityReport->backendAvailable = true;
    if (uint32_t(deviceCreationDesc.profilePolicy) >= uint32_t(ProfilePolicy::MAX_NUM))
        return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if OPENRHI_ENABLE_METAL_SUPPORT
    // Valid adapter expected
    if (!deviceCreationMetalDesc.mtlDevice)
        return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

    if (!GetAdapterDescMetal(adapterDesc, deviceCreationMetalDesc.mtlDevice))
        return ReportCreationFailure(deviceCreationDesc, Result::UNSUPPORTED);

    // Valid queue families expected (native queues are matched by family index)
    Vector<QueueFamilyDesc> queueFamilies(deviceCreationDesc.allocationCallbacks);

    for (uint32_t i = 0; i < deviceCreationMetalDesc.queueFamilyNum; i++) {
        const QueueFamilyMetalDesc& queueFamilyMetalDesc = deviceCreationMetalDesc.queueFamilies[i];

        uint32_t queueType = (uint32_t)queueFamilyMetalDesc.queueType;

        if (queueType >= (uint32_t)QueueType::MAX_NUM)
            return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

        QueueFamilyDesc queueFamily = {};
        queueFamily.queueType = queueFamilyMetalDesc.queueType;
        queueFamily.queueNum = std::min(queueFamilyMetalDesc.queueNum, adapterDesc.queueNum[queueType]);
        queueFamilies.push_back(queueFamily);
    }

    deviceCreationDesc.queueFamilies = queueFamilies.data();
    deviceCreationDesc.queueFamilyNum = (uint32_t)queueFamilies.size();

    result = CreateDeviceMetal(deviceCreationDesc, deviceCreationMetalDesc, deviceImpl);
#endif

    if (result != Result::SUCCESS) {
        if (deviceCreationDesc.compatibilityReport)
            deviceCreationDesc.compatibilityReport->result = result;
        device = nullptr;
        return result;
    }

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

#endif

#if OPENRHI_ENABLE_D3D12_SUPPORT
OPENRHI_API Result OPENRHI_CALL openrhiCreateDeviceFromD3D12Device(const DeviceCreationD3D12Desc& deviceCreationD3D12Desc, Device*& device) {
    DeviceCreationD3D12Desc modifiedDeviceCreationD3D12Desc = deviceCreationD3D12Desc;

    DeviceCreationDesc deviceCreationDesc = {};
    device = nullptr;
    deviceCreationDesc.requirements = deviceCreationD3D12Desc.requirements;
    deviceCreationDesc.compatibilityReport = deviceCreationD3D12Desc.compatibilityReport;
    deviceCreationDesc.profilePolicy = deviceCreationD3D12Desc.profilePolicy;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D12;

    AdapterDesc adapterDesc = {};
    deviceCreationDesc.adapterDesc = &adapterDesc;

    // Copy what is possible to the main "desc"
    deviceCreationDesc.callbackInterface = deviceCreationD3D12Desc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationD3D12Desc.allocationCallbacks;
    deviceCreationDesc.d3dShaderExtRegister = deviceCreationD3D12Desc.d3dShaderExtRegister;
    deviceCreationDesc.d3dZeroBufferSize = deviceCreationD3D12Desc.d3dZeroBufferSize;
    deviceCreationDesc.enableOpenRHIValidation = deviceCreationD3D12Desc.enableOpenRHIValidation;
    deviceCreationDesc.enableMemoryZeroInitialization = deviceCreationD3D12Desc.enableMemoryZeroInitialization;
    deviceCreationDesc.disableD3D12EnhancedBarriers = deviceCreationD3D12Desc.disableD3D12EnhancedBarriers;

    CheckAndSetDefaultCallbacks(deviceCreationDesc);
    InitializeCompatibilityReport(deviceCreationDesc);
    if (!IsBackendAvailable(deviceCreationDesc.graphicsAPI))
        return ReportCreationFailure(deviceCreationDesc, Result::UNSUPPORTED);
    if (deviceCreationDesc.compatibilityReport)
        deviceCreationDesc.compatibilityReport->backendAvailable = true;
    if (uint32_t(deviceCreationDesc.profilePolicy) >= uint32_t(ProfilePolicy::MAX_NUM))
        return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if OPENRHI_ENABLE_D3D12_SUPPORT
    // Valid adapter expected
    if (!deviceCreationD3D12Desc.d3d12Device)
        return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

    LUID luid = deviceCreationD3D12Desc.d3d12Device->GetAdapterLuid();
    uint32_t unused = 0;
    UpdateAdaptersD3D(&adapterDesc, unused, &luid);

    // Valid queue families expected
    Vector<QueueFamilyD3D12Desc> queueFamilies(deviceCreationDesc.allocationCallbacks);
    if (modifiedDeviceCreationD3D12Desc.queueFamilyNum)
        queueFamilies.assign(modifiedDeviceCreationD3D12Desc.queueFamilies, modifiedDeviceCreationD3D12Desc.queueFamilies + modifiedDeviceCreationD3D12Desc.queueFamilyNum);

    modifiedDeviceCreationD3D12Desc.queueFamilies = queueFamilies.data();

    for (uint32_t i = 0; i < modifiedDeviceCreationD3D12Desc.queueFamilyNum; i++) {
        QueueFamilyD3D12Desc& queueFamilyD3D12Desc = queueFamilies[i];

        uint32_t queueType = (uint32_t)queueFamilyD3D12Desc.queueType;
        if (queueType >= (uint32_t)QueueType::MAX_NUM)
            return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

        uint32_t supportedQueueNum = deviceCreationDesc.adapterDesc->queueNum[queueType];
        if (queueFamilyD3D12Desc.queueNum > supportedQueueNum)
            queueFamilyD3D12Desc.queueNum = supportedQueueNum;
    }

    result = CreateDeviceD3D12(deviceCreationDesc, modifiedDeviceCreationD3D12Desc, deviceImpl);
#endif

    if (result != Result::SUCCESS) {
        if (deviceCreationDesc.compatibilityReport)
            deviceCreationDesc.compatibilityReport->result = result;
        device = nullptr;
        return result;
    }

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

#endif

#if OPENRHI_ENABLE_VK_SUPPORT
OPENRHI_API Result OPENRHI_CALL openrhiCreateDeviceFromVKDevice(const DeviceCreationVKDesc& deviceCreationVKDesc, Device*& device) {
    DeviceCreationVKDesc modifiedDeviceCreationVKDesc = deviceCreationVKDesc;

    DeviceCreationDesc deviceCreationDesc = {};
    device = nullptr;
    deviceCreationDesc.requirements = deviceCreationVKDesc.requirements;
    deviceCreationDesc.compatibilityReport = deviceCreationVKDesc.compatibilityReport;
    deviceCreationDesc.profilePolicy = deviceCreationVKDesc.profilePolicy;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::VK;

    AdapterDesc adapterDesc = {};
    deviceCreationDesc.adapterDesc = &adapterDesc;

    // Copy what is possible to the main "desc"
    deviceCreationDesc.callbackInterface = deviceCreationVKDesc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationVKDesc.allocationCallbacks;
    deviceCreationDesc.enableOpenRHIValidation = deviceCreationVKDesc.enableOpenRHIValidation;
    deviceCreationDesc.enableMemoryZeroInitialization = deviceCreationVKDesc.enableMemoryZeroInitialization;
    deviceCreationDesc.vkBindingOffsets = deviceCreationVKDesc.vkBindingOffsets;
    deviceCreationDesc.vkExtensions = deviceCreationVKDesc.vkExtensions;

    CheckAndSetDefaultCallbacks(deviceCreationDesc);
    InitializeCompatibilityReport(deviceCreationDesc);
    if (!IsBackendAvailable(deviceCreationDesc.graphicsAPI))
        return ReportCreationFailure(deviceCreationDesc, Result::UNSUPPORTED);
    if (deviceCreationDesc.compatibilityReport)
        deviceCreationDesc.compatibilityReport->backendAvailable = true;
    if (uint32_t(deviceCreationDesc.profilePolicy) >= uint32_t(ProfilePolicy::MAX_NUM))
        return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if OPENRHI_ENABLE_VK_SUPPORT
    // Valid adapter expected
    if (!deviceCreationVKDesc.vkPhysicalDevice)
        return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

    uint32_t unused = 0;
    UpdateAdaptersVK(&adapterDesc, unused, (VkPhysicalDevice)deviceCreationVKDesc.vkPhysicalDevice);

    // Valid queue families expected
    Vector<QueueFamilyVKDesc> queueFamilies(deviceCreationDesc.allocationCallbacks);
    if (modifiedDeviceCreationVKDesc.queueFamilyNum)
        queueFamilies.assign(modifiedDeviceCreationVKDesc.queueFamilies, modifiedDeviceCreationVKDesc.queueFamilies + modifiedDeviceCreationVKDesc.queueFamilyNum);

    modifiedDeviceCreationVKDesc.queueFamilies = queueFamilies.data();

    for (uint32_t i = 0; i < modifiedDeviceCreationVKDesc.queueFamilyNum; i++) {
        QueueFamilyVKDesc& queueFamilyVKDesc = queueFamilies[i];

        uint32_t queueType = (uint32_t)queueFamilyVKDesc.queueType;
        if (queueType >= (uint32_t)QueueType::MAX_NUM)
            return ReportCreationFailure(deviceCreationDesc, Result::INVALID_ARGUMENT);

        uint32_t supportedQueueNum = deviceCreationDesc.adapterDesc->queueNum[queueType];
        if (queueFamilyVKDesc.queueNum > supportedQueueNum)
            queueFamilyVKDesc.queueNum = supportedQueueNum;
    }

    result = CreateDeviceVK(deviceCreationDesc, modifiedDeviceCreationVKDesc, deviceImpl);
#endif

    if (result != Result::SUCCESS) {
        if (deviceCreationDesc.compatibilityReport)
            deviceCreationDesc.compatibilityReport->result = result;
        device = nullptr;
        return result;
    }

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

#endif

OPENRHI_API void OPENRHI_CALL openrhiDestroyDevice(Device* device) {
    if (device)
        ((DeviceBase*)device)->Destruct();
}

OPENRHI_API Format OPENRHI_CALL openrhiConvertVKFormatToOpenRHI(uint32_t vkFormat) {
    return VKFormatToOpenRHIFormat(vkFormat);
}

OPENRHI_API Format OPENRHI_CALL openrhiConvertDXGIFormatToOpenRHI(uint32_t dxgiFormat) {
    return DXGIFormatToOpenRHIFormat(dxgiFormat);
}

OPENRHI_API uint32_t OPENRHI_CALL openrhiConvertOpenRHIFormatToVK(Format format) {
    MaybeUnused(format);

#if OPENRHI_ENABLE_VK_SUPPORT
    return ConvertFormatToVK(format);
#else
    return 0;
#endif
}

OPENRHI_API uint32_t OPENRHI_CALL openrhiConvertOpenRHIFormatToDXGI(Format format) {
    MaybeUnused(format);

#if OPENRHI_ENABLE_D3D12_SUPPORT
    return ConvertFormatToDXGI(format);
#else
    return 0;
#endif
}

OPENRHI_API const FormatProps* OPENRHI_CALL openrhiGetFormatProps(Format format) {
    return &GetFormatProps(format);
}

OPENRHI_API const char* OPENRHI_CALL openrhiGetGraphicsAPIString(GraphicsAPI graphicsAPI) {
    switch (graphicsAPI) {
        case GraphicsAPI::NONE:
            return "NONE";
        case GraphicsAPI::D3D12:
            return "D3D12";
        case GraphicsAPI::VK:
            return "VK";
        case GraphicsAPI::METAL:
            return "METAL";
        default:
            return "UNKNOWN";
    }
}

OPENRHI_API Result OPENRHI_CALL openrhiEnumerateAdapters(AdapterDesc* outAdapterDescs, uint32_t& outAdapterDescNum) {
    std::array<AdapterDesc, ADAPTER_MAX_NUM> adapterDescs = {};
    uint32_t adapterDescNum = 0;

#if OPENRHI_ENABLE_VK_SUPPORT
    UpdateAdaptersVK(adapterDescs.data(), adapterDescNum, nullptr);
#endif

#if OPENRHI_ENABLE_D3D12_SUPPORT
    UpdateAdaptersD3D(adapterDescs.data(), adapterDescNum, nullptr);
#endif

#if OPENRHI_ENABLE_METAL_SUPPORT
    UpdateAdaptersMetal(adapterDescs.data(), adapterDescNum, ADAPTER_MAX_NUM);
#endif

#if OPENRHI_ENABLE_NONE_SUPPORT
    if (!adapterDescNum) {
        AdapterDesc& adapterDesc = adapterDescs[adapterDescNum++];

        adapterDesc.videoMemorySize = 128ull << 30;
        adapterDesc.sharedSystemMemorySize = 128ull << 30;

        for (uint32_t i = 0; i < GetCountOf(adapterDesc.queueNum); i++)
            adapterDesc.queueNum[i] = 32;

        strncpy(adapterDesc.name, "NONE", sizeof(adapterDesc.name));
    }
#endif

    // Sort by video memory size and arhitecture (DISCRETE first)
    qsort(adapterDescs.data(), adapterDescNum, sizeof(adapterDescs[0]), SortAdapters);

    // Copy to output
    if (outAdapterDescs) {
        outAdapterDescNum = std::min(outAdapterDescNum, adapterDescNum);
        for (uint32_t i = 0; i < outAdapterDescNum; i++) {
            outAdapterDescs[i] = adapterDescs[i];
#if OPENRHI_ENABLE_NONE_SUPPORT
            outAdapterDescs[i].supportedGraphicsAPIs |= GraphicsAPI::NONE;
#endif
        }
    } else
        outAdapterDescNum = adapterDescNum;

    return outAdapterDescNum == 0 ? Result::UNSUPPORTED : Result::SUCCESS;
}

OPENRHI_API void OPENRHI_CALL openrhiReportLiveObjects() {
#if OPENRHI_ENABLE_D3D12_SUPPORT
    ComPtr<IDXGIDebug1> pDebug;
    HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug));
    if (SUCCEEDED(hr))
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)((uint32_t)DXGI_DEBUG_RLO_DETAIL | (uint32_t)DXGI_DEBUG_RLO_IGNORE_INTERNAL));
#endif
}

OPENRHI_API Result OPENRHI_CALL openrhiReportDeviceLostInfo(Device& device, DeviceLostDump& deviceLostDump) {
    return ((DeviceBase&)device).ReportDeviceLostInfo(deviceLostDump);
}

OPENRHI_API PipelineLayoutSettingsDesc OPENRHI_CALL openrhiFitPipelineLayoutSettingsIntoDeviceLimits(const DeviceDesc& deviceDesc, const PipelineLayoutSettingsDesc& pipelineLayoutSettingsDesc) {
    // D3D12 root signature costs and the cross-API root descriptor size, in bytes
    const uint32_t rootConstantCost = 4;
    const uint32_t descriptorTableCost = 4;
    const uint32_t rootDescriptorCost = 8;

    uint32_t descriptorSetNum = pipelineLayoutSettingsDesc.descriptorSetNum;
    uint32_t descriptorRangeNum = pipelineLayoutSettingsDesc.descriptorRangeNum;
    uint32_t rootConstantSize = pipelineLayoutSettingsDesc.rootConstantSize;
    uint32_t rootDescriptorNum = pipelineLayoutSettingsDesc.rootDescriptorNum;
    uint32_t rootSamplerNum = pipelineLayoutSettingsDesc.rootSamplerNum;

    if (pipelineLayoutSettingsDesc.useDescriptorHeap) {
        // "Descriptor heap" mode has no descriptor sets or ranges and uses its own root parameter limits
        descriptorSetNum = 0;
        descriptorRangeNum = 0;

        if (rootDescriptorNum > deviceDesc.descriptorHeap.rootDescriptorMaxNum)
            rootDescriptorNum = deviceDesc.descriptorHeap.rootDescriptorMaxNum;

        if (rootSamplerNum > deviceDesc.descriptorHeap.rootSamplerMaxNum)
            rootSamplerNum = deviceDesc.descriptorHeap.rootSamplerMaxNum;

        uint32_t freeRootDataSize = deviceDesc.descriptorHeap.rootConstantMaxSize;
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            // D3D12 emulation injects root parameters that consume the same root-data budget
            uint32_t reservedRootDataSize = 0;

            if (pipelineLayoutSettingsDesc.enableD3D12DrawParametersEmulation)
                reservedRootDataSize += rootDescriptorCost;

            if (pipelineLayoutSettingsDesc.enableD3D12DrawIndexEmulation)
                reservedRootDataSize += rootConstantCost;

            freeRootDataSize = (reservedRootDataSize < freeRootDataSize) ? freeRootDataSize - reservedRootDataSize : 0;
        }

        // Root constants and descriptors share the root-data budget; fit them in the requested priority order
        if (pipelineLayoutSettingsDesc.preferRootDescriptorsOverConstants) {
            const uint32_t availableRootDescriptorNum = freeRootDataSize / rootDescriptorCost;
            if (rootDescriptorNum > availableRootDescriptorNum)
                rootDescriptorNum = availableRootDescriptorNum;

            freeRootDataSize -= rootDescriptorNum * rootDescriptorCost;
            const uint32_t availableRootConstantSize = rootDescriptorNum ? freeRootDataSize / rootDescriptorCost * rootDescriptorCost : freeRootDataSize;
            if (rootConstantSize > availableRootConstantSize)
                rootConstantSize = availableRootConstantSize;
        } else {
            if (rootConstantSize > freeRootDataSize)
                rootConstantSize = freeRootDataSize;

            const uint32_t alignedRootConstantSize = Align(rootConstantSize, rootDescriptorCost);
            const uint32_t availableRootDescriptorNum = (alignedRootConstantSize <= freeRootDataSize) ? (freeRootDataSize - alignedRootConstantSize) / rootDescriptorCost : 0;
            if (rootDescriptorNum > availableRootDescriptorNum)
                rootDescriptorNum = availableRootDescriptorNum;
        }
    } else {
        // Pipeline layouts using descriptor pools have independent root parameter limits
        if (rootConstantSize > deviceDesc.pipelineLayout.rootConstantMaxSize)
            rootConstantSize = deviceDesc.pipelineLayout.rootConstantMaxSize;

        if (rootDescriptorNum > deviceDesc.pipelineLayout.rootDescriptorMaxNum)
            rootDescriptorNum = deviceDesc.pipelineLayout.rootDescriptorMaxNum;

        if (rootSamplerNum > deviceDesc.pipelineLayout.rootSamplerMaxNum)
            rootSamplerNum = deviceDesc.pipelineLayout.rootSamplerMaxNum;

        // Vulkan root descriptors and root samplers share the push-descriptor limit
        if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
            const uint32_t availableRootSamplerNum = (rootDescriptorNum < deviceDesc.pipelineLayout.rootSamplerMaxNum) ? (deviceDesc.pipelineLayout.rootSamplerMaxNum - rootDescriptorNum) : 0;
            if (rootSamplerNum > availableRootSamplerNum)
                rootSamplerNum = availableRootSamplerNum;
        }

        uint32_t pipelineLayoutDescriptorSetMaxNum = deviceDesc.pipelineLayout.descriptorSetMaxNum;

        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            // Descriptor tables, root constants, root descriptors and emulation parameters share the root signature budget
            uint32_t freeBytesInRootSignature = 256;

            if (pipelineLayoutSettingsDesc.enableD3D12DrawParametersEmulation)
                freeBytesInRootSignature -= rootDescriptorCost;

            if (pipelineLayoutSettingsDesc.enableD3D12DrawIndexEmulation)
                freeBytesInRootSignature -= rootConstantCost;

            uint32_t availableDescriptorRangeNum = freeBytesInRootSignature / descriptorTableCost;
            if (descriptorRangeNum > availableDescriptorRangeNum)
                descriptorRangeNum = availableDescriptorRangeNum;

            freeBytesInRootSignature -= descriptorRangeNum * descriptorTableCost;

            if (pipelineLayoutSettingsDesc.preferRootDescriptorsOverConstants) {
                uint32_t availableRootDescriptorNum = freeBytesInRootSignature / rootDescriptorCost;
                if (rootDescriptorNum > availableRootDescriptorNum)
                    rootDescriptorNum = availableRootDescriptorNum;

                freeBytesInRootSignature -= rootDescriptorNum * rootDescriptorCost;

                if (rootConstantSize > freeBytesInRootSignature)
                    rootConstantSize = freeBytesInRootSignature;
            } else {
                if (rootConstantSize > freeBytesInRootSignature)
                    rootConstantSize = freeBytesInRootSignature;

                freeBytesInRootSignature -= rootConstantSize;

                uint32_t availableRootDescriptorNum = freeBytesInRootSignature / rootDescriptorCost;
                if (rootDescriptorNum > availableRootDescriptorNum)
                    rootDescriptorNum = availableRootDescriptorNum;
            }
        } else {
            // Other backends reserve one internal descriptor set for root descriptors and samplers
            if ((rootDescriptorNum || rootSamplerNum) && pipelineLayoutDescriptorSetMaxNum)
                pipelineLayoutDescriptorSetMaxNum--;
        }

        if (descriptorSetNum > pipelineLayoutDescriptorSetMaxNum)
            descriptorSetNum = pipelineLayoutDescriptorSetMaxNum;
    }

    // Preserve preferences and report only the fitted quantities
    PipelineLayoutSettingsDesc modifiedPipelineLayoutLimitsDesc = pipelineLayoutSettingsDesc;
    modifiedPipelineLayoutLimitsDesc.descriptorSetNum = descriptorSetNum;
    modifiedPipelineLayoutLimitsDesc.descriptorRangeNum = descriptorRangeNum;
    modifiedPipelineLayoutLimitsDesc.rootConstantSize = rootConstantSize;
    modifiedPipelineLayoutLimitsDesc.rootDescriptorNum = rootDescriptorNum;
    modifiedPipelineLayoutLimitsDesc.rootSamplerNum = rootSamplerNum;

    return modifiedPipelineLayoutLimitsDesc;
}

OPENRHI_API Result OPENRHI_CALL openrhiEvaluateDeviceSupport(const DeviceDesc& desc, const DeviceRequirements& requirements, DeviceCompatibilityReport& report) {
    return EvaluateDeviceSupport(desc, requirements, report);
}

OPENRHI_API const DeviceProfileDesc* OPENRHI_CALL openrhiGetDeviceProfile(const Device& device) {
    return &((const DeviceBase&)device).GetDeviceProfile();
}

OPENRHI_API Result OPENRHI_CALL openrhiQueryAdapterSupport(const DeviceCreationDesc& desc, DeviceCompatibilityReport& report) {
    DeviceCreationDesc probe = desc;
    probe.compatibilityReport = &report;
    probe.enableOpenRHIValidation = false;
    Device* device = nullptr;
    Result result = openrhiCreateDevice(probe, device);
    openrhiDestroyDevice(device);
    report.result = result;
    return result;
}

OPENRHI_API DeviceRequirements OPENRHI_CALL openrhiGetPlatformRequirements(GraphicsAPI api) {
    DeviceRequirements requirements = {};
    requirements.requiredCapabilities = GetPlatformRequirements(api);
    requirements.minShaderModel = api == GraphicsAPI::D3D12 ? OpenRhiShaderModel(6, 6) : 0;
    return requirements;
}
