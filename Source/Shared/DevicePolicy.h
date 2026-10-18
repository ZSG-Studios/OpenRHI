// OpenRHI creation-time requirements and conservative device policies
#pragma once

#include "OpenRHI.h"

#include "Extensions/OpenRHIDeviceCreation.h"

namespace openrhi {

static inline DeviceCapabilityBits GetPlatformRequirements(GraphicsAPI api) {
    using B = DeviceCapabilityBits;
    switch (api) {
        case GraphicsAPI::D3D12:
            return B::D3D12_FEATURE_LEVEL_12_2 | B::SHADER_MODEL_6_6 | B::ENHANCED_BARRIERS;
        case GraphicsAPI::VK:
            return B::VULKAN_1_3 | B::DYNAMIC_RENDERING | B::SYNCHRONIZATION_2 | B::MAINTENANCE_4 | B::BUFFER_DEVICE_ADDRESS | B::TIMELINE_SEMAPHORE | B::RUNTIME_DESCRIPTOR_ARRAY | B::PARTIALLY_BOUND_DESCRIPTORS | B::SAMPLED_IMAGE_NONUNIFORM_INDEXING;
        case GraphicsAPI::METAL:
            return B::METAL_4 | B::APPLE_GPU_FAMILY_9;
        default:
            return B::NONE;
    }
}

static inline Result EvaluateDeviceSupport(const DeviceDesc& desc, const DeviceRequirements& requirements, DeviceCompatibilityReport& report) {
    report = {};
    report.graphicsAPI = desc.graphicsAPI;
    report.adapterUid = desc.adapterDesc.uid;
    report.backendAvailable = desc.graphicsAPI == GraphicsAPI::D3D12 || desc.graphicsAPI == GraphicsAPI::VK || desc.graphicsAPI == GraphicsAPI::METAL || desc.graphicsAPI == GraphicsAPI::NONE;
    report.requiredCapabilities = GetPlatformRequirements(desc.graphicsAPI) | requirements.requiredCapabilities;
    report.supportedCapabilities = desc.nativeCapabilities;
    report.missingCapabilities = DeviceCapabilityBits(report.requiredCapabilities & ~desc.nativeCapabilities);
    report.requiredMemoryBudget = requirements.minMemoryBudget;
    report.availableMemoryBudget = desc.adapterDesc.memoryBudget;
    report.availableMemoryBudgetKnown = desc.adapterDesc.memoryBudgetKnown;
    report.requiredShaderModel = requirements.minShaderModel;
    if (desc.graphicsAPI == GraphicsAPI::D3D12 && report.requiredShaderModel < OpenRhiShaderModel(6, 6))
        report.requiredShaderModel = OpenRhiShaderModel(6, 6);

    report.supportedShaderModel = desc.shaderModel;
    report.memoryBudgetSatisfied = (report.requiredMemoryBudget == 0 || (report.availableMemoryBudgetKnown && report.availableMemoryBudget >= report.requiredMemoryBudget));
    report.shaderModelSatisfied = report.supportedShaderModel >= report.requiredShaderModel;
    report.result = uint32_t(report.missingCapabilities) == 0 && report.memoryBudgetSatisfied && report.shaderModelSatisfied ? Result::SUCCESS : Result::UNSUPPORTED;
    if (!report.backendAvailable)
        report.result = Result::INVALID_ARGUMENT;
    return report.result;
}

static inline void InitializeCompatibilityReport(const DeviceCreationDesc& creation) {
    if (!creation.compatibilityReport)
        return;
    DeviceCompatibilityReport& report = *creation.compatibilityReport;
    report = {};
    report.result = Result::UNSUPPORTED;
    report.graphicsAPI = creation.graphicsAPI;
    report.requiredCapabilities = GetPlatformRequirements(creation.graphicsAPI);
    if (creation.requirements) {
        report.requiredCapabilities |= creation.requirements->requiredCapabilities;
        report.requiredMemoryBudget = creation.requirements->minMemoryBudget;
        report.requiredShaderModel = creation.requirements->minShaderModel;
    }
    if (creation.graphicsAPI == GraphicsAPI::D3D12 && report.requiredShaderModel < OpenRhiShaderModel(6, 6))
        report.requiredShaderModel = OpenRhiShaderModel(6, 6);
    report.missingCapabilities = report.requiredCapabilities;
    if (creation.adapterDesc)
        report.adapterUid = creation.adapterDesc->uid;
}

static inline Result CheckDeviceRequirements(const DeviceCreationDesc& creation, const DeviceDesc& desc) {
    DeviceCompatibilityReport local = {};
    DeviceCompatibilityReport& report = creation.compatibilityReport ? *creation.compatibilityReport : local;
    const DeviceRequirements empty = {};
    return EvaluateDeviceSupport(desc, creation.requirements ? *creation.requirements : empty, report);
}

static inline bool IsAdapterPreferred(const AdapterDesc& a, const AdapterDesc& b) {
    const bool aDiscrete = a.architecture == Architecture::DISCRETE;
    const bool bDiscrete = b.architecture == Architecture::DISCRETE;
    if (aDiscrete != bDiscrete)
        return aDiscrete;

    if (a.memoryBudget != b.memoryBudget)
        return a.memoryBudget > b.memoryBudget;

    if (a.uid.high != b.uid.high)
        return a.uid.high < b.uid.high;

    return a.uid.low < b.uid.low;
}

static inline GpuFamily DetectGpuFamily(const AdapterDesc& adapter) {
    // Exact PC device IDs only; the source receipts are recorded in Profiles/targets.json
    if (adapter.vendorId == 0x1002 && adapter.deviceId == 0x7550)
        return GpuFamily::AMD_RDNA_4;

    if (adapter.vendorId == 0x10de && (adapter.deviceId == 0x2b85 || adapter.deviceId == 0x2c02))
        return GpuFamily::NVIDIA_BLACKWELL;

    if (adapter.vendorId == 0x8086 && (adapter.deviceId == 0xe20b || adapter.deviceId == 0xe20c))
        return GpuFamily::INTEL_XE_2;

    if (adapter.vendorId == 0x8086) {
        const uint32_t xe3[] = {0xb080, 0xb081, 0xb082, 0xb083, 0xb084, 0xb085, 0xb086, 0xb087, 0xb08f, 0xb0a0, 0xb0a1, 0xb0b0, 0xb090};
        for (uint32_t id : xe3)
            if (adapter.deviceId == id)
                return GpuFamily::INTEL_XE_3;
    }
    if (adapter.appleGpuFamily >= 10)
        return GpuFamily::APPLE_10;

    if (adapter.appleGpuFamily >= 9)
        return GpuFamily::APPLE_9;

    if (adapter.vendorId == 0x5143)
        return GpuFamily::QUALCOMM_ADRENO;

    if (adapter.vendorId == 0x13b5)
        return GpuFamily::ARM_MALI;

    if (adapter.vendorId == 0x144d)
        return GpuFamily::SAMSUNG_XCLIPSE;

    return GpuFamily::UNKNOWN;
}

static inline DeviceProfileDesc ResolveDeviceProfile(const DeviceDesc& desc, ProfilePolicy requested) {
    DeviceProfileDesc profile = {};
    profile.revision = 1;
    profile.detectedFamily = DetectGpuFamily(desc.adapterDesc);
    profile.policy = requested;
    profile.qualification = QualificationStatus::UNQUALIFIED;
    // All architectures start with conservative generic policies; promotion requires receipts
    profile.unifiedMemory = desc.adapterDesc.architecture == Architecture::INTEGRATED;
    profile.uploadPolicy = profile.unifiedMemory ? UploadPolicy::UNIFIED_MEMORY : UploadPolicy::STAGING_COPY;
    profile.preferVulkan14 = (desc.nativeCapabilities & DeviceCapabilityBits::VULKAN_1_4) != 0;
    profile.recommendedGraphicsQueues = desc.adapterDesc.queueNum[uint32_t(QueueType::GRAPHICS)] ? 1 : 0;
    profile.recommendedComputeQueues = 0;
    profile.recommendedCopyQueues = 0;
    return profile;
}

} // namespace openrhi
