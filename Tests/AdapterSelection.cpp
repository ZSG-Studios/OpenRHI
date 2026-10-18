#include "OpenRHI.h"

#if OPENRHI_ENABLE_AGILITY_SDK_SUPPORT
#    include "OpenRHIAgilitySDK.h"
#endif

#include "Extensions/OpenRHIDeviceCreation.h"

#include <cstdio>
#include <vector>

using namespace openrhi;

static void OPENRHI_CALL MessageCallback(Message, const char*, uint32_t, const char*, void*) {
}

static void OPENRHI_CALL NoAbort(void*) {
}

static bool Same(Uid_t a, Uid_t b) {
    return a.low == b.low && a.high == b.high;
}

int main() {
#if defined(_WIN32)
    const GraphicsAPI api = GraphicsAPI::D3D12;
#elif defined(__APPLE__)
    const GraphicsAPI api = GraphicsAPI::METAL;
#else
    const GraphicsAPI api = GraphicsAPI::VK;
#endif
    DeviceCreationDesc creation = {};
    creation.graphicsAPI = api;
    creation.callbackInterface = {MessageCallback, NoAbort, nullptr};
    DeviceCompatibilityReport report = {};
    creation.compatibilityReport = &report;
    uint32_t count = 0;
    if (openrhiEnumerateAdapters(nullptr, count) != Result::SUCCESS)
        return 1;
    std::vector<AdapterDesc> adapters(count);
    if (openrhiEnumerateAdapters(adapters.data(), count) != Result::SUCCESS)
        return 1;
    const AdapterDesc* compatible = nullptr;
    for (const auto& adapter : adapters) {
        if (!(adapter.supportedGraphicsAPIs & api))
            continue;
        creation.adapterDesc = &adapter;
        Result result = openrhiQueryAdapterSupport(creation, report);
        std::printf("{\"schema\":1,\"vendor_id\":%u,\"device_id\":%u,\"uid_low\":%llu,\"uid_high\":%llu,\"driver\":%llu,\"result\":%u,\"supported\":%u,\"required\":%u,\"missing\":%u,\"shader_model\":%u,\"memory_budget\":%llu}\n", adapter.vendorId, adapter.deviceId, (unsigned long long)adapter.uid.low, (unsigned long long)adapter.uid.high, (unsigned long long)adapter.nativeDriverVersion, uint32_t(result), uint32_t(report.supportedCapabilities), uint32_t(report.requiredCapabilities), uint32_t(report.missingCapabilities), uint32_t(report.supportedShaderModel), (unsigned long long)report.availableMemoryBudget);
        if (result != report.result || !Same(adapter.uid, report.adapterUid))
            return 2;
        if (result == Result::SUCCESS && adapter.architecture != Architecture::SOFTWARE && !compatible)
            compatible = &adapter;
    }
    if (!compatible)
        return 77;
    Device* device = nullptr;
    creation.adapterDesc = compatible;
    creation.profilePolicy = ProfilePolicy::GENERIC;
    if (openrhiCreateDevice(creation, device) != Result::SUCCESS || !device)
        return 3;
    const auto& profile = *openrhiGetDeviceProfile(*device);
    if (profile.policy != ProfilePolicy::GENERIC || profile.tunedPoliciesEnabled)
        return 4;
    openrhiDestroyDevice(device);
    DeviceRequirements impossible = {};
    impossible.minMemoryBudget = UINT64_MAX;
    creation.requirements = &impossible;
    device = (Device*)uintptr_t(1);
    if (openrhiCreateDevice(creation, device) != Result::UNSUPPORTED || device)
        return 5;
    if (!Same(report.adapterUid, compatible->uid) || report.memoryBudgetSatisfied || report.result != Result::UNSUPPORTED)
        return 6;
    creation.requirements = nullptr;
#if defined(_WIN32)
    creation.disableD3D12EnhancedBarriers = true;
    if (openrhiQueryAdapterSupport(creation, report) != Result::UNSUPPORTED)
        return 7;
    if (!(report.missingCapabilities & DeviceCapabilityBits::ENHANCED_BARRIERS))
        return 8;
    creation.disableD3D12EnhancedBarriers = false;
#endif
    AdapterDesc stale = *compatible;
    stale.uid.low = UINT64_MAX;
    stale.uid.high = UINT64_MAX;
    creation.adapterDesc = &stale;
    device = nullptr;
    if (openrhiCreateDevice(creation, device) == Result::SUCCESS || device || !Same(report.adapterUid, stale.uid))
        return 9;
    creation.adapterDesc = nullptr;
    if (openrhiCreateDevice(creation, device) != Result::SUCCESS || !device)
        return 10;
    if (!Same(report.adapterUid, compatible->uid))
        return 11;
    openrhiDestroyDevice(device);
    std::printf("Explicit selection, rejection, probe, and automatic selection passed\n");
    return 0;
}
