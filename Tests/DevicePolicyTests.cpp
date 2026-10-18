#include "OpenRHI.h"

#include "DevicePolicy.h"
#include "Extensions/OpenRHIDeviceCreation.h"

#include <cstdio>

using namespace openrhi;
static int failures = 0;
#define CHECK(x) \
    do { \
        if (!(x)) { \
            std::printf("FAIL line %d: %s\n", __LINE__, #x); \
            failures++; \
        } \
    } while (0)

int main() {
    DeviceDesc desc = {};
    desc.graphicsAPI = GraphicsAPI::D3D12;
    desc.nativeCapabilities = GetPlatformRequirements(desc.graphicsAPI);
    desc.shaderModel = OpenRhiShaderModel(6, 6);
    desc.adapterDesc.memoryBudget = 1024;
    desc.adapterDesc.memoryBudgetKnown = true;
    DeviceRequirements requirements = {};
    DeviceCompatibilityReport report = {};
    CHECK(EvaluateDeviceSupport(desc, requirements, report) == Result::SUCCESS);
    CHECK(uint32_t(report.missingCapabilities) == 0);
    requirements.minMemoryBudget = 1025;
    CHECK(EvaluateDeviceSupport(desc, requirements, report) == Result::UNSUPPORTED);
    CHECK(!report.memoryBudgetSatisfied);
    requirements = {};
    requirements.minShaderModel = OpenRhiShaderModel(6, 9);
    CHECK(EvaluateDeviceSupport(desc, requirements, report) == Result::UNSUPPORTED);
    CHECK(!report.shaderModelSatisfied);
    requirements = {};
    desc.nativeCapabilities = DeviceCapabilityBits::NONE;
    CHECK(EvaluateDeviceSupport(desc, requirements, report) == Result::UNSUPPORTED);
    CHECK(report.requiredCapabilities == GetPlatformRequirements(GraphicsAPI::D3D12));
    desc.graphicsAPI = GraphicsAPI::VK;
    const uint32_t baseline = uint32_t(GetPlatformRequirements(GraphicsAPI::VK));
    for (uint32_t bit = 1; bit <= uint32_t(DeviceCapabilityBits::APPLE_GPU_FAMILY_10); bit <<= 1) {
        if (!(bit & baseline))
            continue;
        desc.nativeCapabilities = DeviceCapabilityBits(baseline & ~bit);
        CHECK(EvaluateDeviceSupport(desc, requirements, report) == Result::UNSUPPORTED);
        CHECK(uint32_t(report.missingCapabilities) == bit);
    }
    desc.nativeCapabilities = DeviceCapabilityBits(baseline);
    CHECK(EvaluateDeviceSupport(desc, requirements, report) == Result::SUCCESS);
    CHECK(!ResolveDeviceProfile(desc, ProfilePolicy::AUTOMATIC).preferVulkan14);
    desc.nativeCapabilities |= DeviceCapabilityBits::VULKAN_1_4;
    CHECK(ResolveDeviceProfile(desc, ProfilePolicy::AUTOMATIC).preferVulkan14);
    desc.graphicsAPI = GraphicsAPI::METAL;
    desc.nativeCapabilities = DeviceCapabilityBits::METAL_4;
    CHECK(EvaluateDeviceSupport(desc, requirements, report) == Result::UNSUPPORTED);
    desc.nativeCapabilities |= DeviceCapabilityBits::APPLE_GPU_FAMILY_9;
    CHECK(EvaluateDeviceSupport(desc, requirements, report) == Result::SUCCESS);
    AdapterDesc discrete = {}, integrated = {};
    discrete.architecture = Architecture::DISCRETE;
    integrated.architecture = Architecture::INTEGRATED;
    integrated.memoryBudget = 10000;
    CHECK(IsAdapterPreferred(discrete, integrated));
    AdapterDesc a = discrete, b = discrete;
    a.memoryBudget = 2;
    b.memoryBudget = 1;
    CHECK(IsAdapterPreferred(a, b));
    b.memoryBudget = 2;
    a.uid.low = 1;
    b.uid.low = 2;
    CHECK(IsAdapterPreferred(a, b));
    CHECK(!IsAdapterPreferred(a, a));
    a.vendorId = 0x1002;
    a.deviceId = 0x7550;
    CHECK(DetectGpuFamily(a) == GpuFamily::AMD_RDNA_4);
    a.deviceId = 0x164e;
    CHECK(DetectGpuFamily(a) == GpuFamily::UNKNOWN);
    desc.adapterDesc = a;
    auto profile = ResolveDeviceProfile(desc, ProfilePolicy::AUTOMATIC);
    CHECK(!profile.tunedPoliciesEnabled);
    CHECK(profile.qualification == QualificationStatus::UNQUALIFIED);
    CHECK(profile.recommendedWaveSize == 0);
    profile = ResolveDeviceProfile(desc, ProfilePolicy::GENERIC);
    CHECK(profile.policy == ProfilePolicy::GENERIC);
    std::printf("Device policy tests: %d failures\n", failures);
    return failures ? 1 : 0;
}
