#include "SharedExternal.h"
#include "UpscalerInterface.h"

#include <malloc.h>
#include <cstdio>
#include <cstdlib>

using namespace openrhi;

// Compile the production adapter against the pinned public headers and test SDK entry points
#include "UpscalerInterface.hpp"

enum class FailurePoint {
    INIT,
    PARAMETERS,
    PARTIAL_PARAMETERS,
    FEATURE,
    PARTIAL_FEATURE,
    NONE
};

struct SdkCalls {
    uint32_t init = 0;
    uint32_t parameters = 0;
    uint32_t feature = 0;
    uint32_t release = 0;
    uint32_t destroyParameters = 0;
    uint32_t shutdown = 0;
    uint32_t invalid = 0;
} g_calls;

static FailurePoint g_failure = FailurePoint::NONE;
static uint64_t g_parameterStorage;
static uint64_t g_handleStorage;

NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_D3D12_Init(unsigned long long, const wchar_t*, ID3D12Device*, const NVSDK_NGX_FeatureCommonInfo*, NVSDK_NGX_Version) {
    g_calls.init++;

    return g_failure == FailurePoint::INIT ? NVSDK_NGX_Result_Fail : NVSDK_NGX_Result_Success;
}

NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_D3D12_GetCapabilityParameters(NVSDK_NGX_Parameter** params) {
    g_calls.parameters++;
    *params = g_failure == FailurePoint::PARAMETERS ? nullptr : (NVSDK_NGX_Parameter*)&g_parameterStorage;

    return (g_failure == FailurePoint::PARAMETERS || g_failure == FailurePoint::PARTIAL_PARAMETERS) ? NVSDK_NGX_Result_Fail : NVSDK_NGX_Result_Success;
}

NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_D3D12_CreateFeature(ID3D12GraphicsCommandList*, NVSDK_NGX_Feature, NVSDK_NGX_Parameter*, NVSDK_NGX_Handle** handle) {
    g_calls.feature++;
    *handle = g_failure == FailurePoint::FEATURE ? nullptr : (NVSDK_NGX_Handle*)&g_handleStorage;

    return (g_failure == FailurePoint::FEATURE || g_failure == FailurePoint::PARTIAL_FEATURE) ? NVSDK_NGX_Result_Fail : NVSDK_NGX_Result_Success;
}

NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_D3D12_ReleaseFeature(NVSDK_NGX_Handle* handle) {
    g_calls.release++;
    g_calls.invalid += handle != (NVSDK_NGX_Handle*)&g_handleStorage;

    return NVSDK_NGX_Result_Success;
}

NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_D3D12_DestroyParameters(NVSDK_NGX_Parameter* params) {
    g_calls.destroyParameters++;
    g_calls.invalid += params != (NVSDK_NGX_Parameter*)&g_parameterStorage;

    return NVSDK_NGX_Result_Success;
}

NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_D3D12_Shutdown1(ID3D12Device*) {
    g_calls.shutdown++;

    return NVSDK_NGX_Result_Success;
}

NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_D3D12_EvaluateFeature_C(ID3D12GraphicsCommandList*, const NVSDK_NGX_Handle*, const NVSDK_NGX_Parameter*, PFN_NVSDK_NGX_ProgressCallback_C) {
    g_calls.invalid++;

    return NVSDK_NGX_Result_Fail;
}

void NVSDK_CONV NVSDK_NGX_Parameter_SetUI(NVSDK_NGX_Parameter*, const char*, unsigned int) {
}

void NVSDK_CONV NVSDK_NGX_Parameter_SetI(NVSDK_NGX_Parameter*, const char*, int) {
}

void NVSDK_CONV NVSDK_NGX_Parameter_SetF(NVSDK_NGX_Parameter*, const char*, float) {
}

void NVSDK_CONV NVSDK_NGX_Parameter_SetVoidPointer(NVSDK_NGX_Parameter*, const char*, void*) {
}

void NVSDK_CONV NVSDK_NGX_Parameter_SetD3d12Resource(NVSDK_NGX_Parameter*, const char*, ID3D12Resource*) {
}

struct TestDevice final : DeviceBase {
    TestDevice(const CallbackInterface& callbacks, const AllocationCallbacks& allocations)
        : DeviceBase(callbacks, allocations) {
        desc.graphicsAPI = GraphicsAPI::D3D12;
        desc.adapterDesc.vendor = Vendor::NVIDIA;
        desc.tiers.rayTracing = 1;
    }

    const DeviceDesc& GetDesc() const override {
        return desc;
    }

    void Destruct() override {
    }

    DeviceDesc desc = {};
};

static void* OPENRHI_CALL AllocateTest(void*, size_t size, size_t alignment) {
    return _aligned_malloc(size, alignment);
}

static void OPENRHI_CALL FreeTest(void*, void* memory) {
    _aligned_free(memory);
}

static const DeviceDesc& OPENRHI_CALL GetDeviceDescTest(const Device& device) {
    return ((const TestDevice&)device).GetDesc();
}

static void* OPENRHI_CALL GetDeviceNativeTest(const Device* device) {
    return (void*)device;
}

static void* OPENRHI_CALL GetCommandBufferNativeTest(const CommandBuffer* commandBuffer) {
    return (void*)commandBuffer;
}

static bool Check(bool condition, const char* message) {
    if (!condition)
        std::printf("FAIL: %s\n", message);

    return condition;
}

int main() {
    const CallbackInterface callbacks = {};
    const AllocationCallbacks allocations = {AllocateTest, nullptr, FreeTest};
    TestDevice device(callbacks, allocations);
    CoreInterface core = {};
    core.GetDeviceDesc = GetDeviceDescTest;
    core.GetDeviceNativeObject = GetDeviceNativeTest;
    core.GetCommandBufferNativeObject = GetCommandBufferNativeTest;
    uint64_t commandBufferStorage = 0;
    UpscalerDesc desc = {};
    desc.upscaleResolution = {1920, 1080};
    desc.mode = UpscalerMode::PERFORMANCE;
    desc.commandBuffer = (CommandBuffer*)&commandBufferStorage;

    for (UpscalerType type : {UpscalerType::DLSR, UpscalerType::DLRR}) {
        desc.type = type;

        for (FailurePoint failure : {FailurePoint::INIT, FailurePoint::PARAMETERS, FailurePoint::PARTIAL_PARAMETERS, FailurePoint::FEATURE, FailurePoint::PARTIAL_FEATURE, FailurePoint::NONE}) {
            g_calls = {};
            g_failure = failure;
            Result result;
            {
                UpscalerImpl upscaler((Device&)device, core);
                result = upscaler.Create(desc);
            }

            const bool initialized = failure != FailurePoint::INIT;
            const bool hasParams = initialized && failure != FailurePoint::PARAMETERS;
            const bool hasFeature = failure == FailurePoint::PARTIAL_FEATURE || failure == FailurePoint::NONE;

            if (!Check((result == Result::SUCCESS) == (failure == FailurePoint::NONE), "creation result")
                || !Check(g_calls.invalid == 0, "null SDK object or unexpected dispatch")
                || !Check(g_calls.destroyParameters == uint32_t(hasParams), "parameter cleanup")
                || !Check(g_calls.release == uint32_t(hasFeature), "feature cleanup")
                || !Check(g_calls.shutdown == uint32_t(initialized), "initialization cleanup"))
                return 1;
        }

        g_calls = {};
        g_failure = FailurePoint::NONE;
        {
            UpscalerImpl first((Device&)device, core);

            if (!Check(first.Create(desc) == Result::SUCCESS, "first shared-device feature"))
                return 1;

            {
                UpscalerImpl second((Device&)device, core);

                if (!Check(second.Create(desc) == Result::SUCCESS, "second shared-device feature"))
                    return 1;
            }

            if (!Check(g_calls.shutdown == 0, "shared-device lifetime"))
                return 1;
        }

        if (!Check(g_calls.shutdown == 1 && g_calls.release == 2 && g_calls.destroyParameters == 2 && g_calls.invalid == 0, "last shared-device cleanup"))
            return 1;
    }

    std::puts("NGX SR/RR failure, retry and shared-device cleanup: PASS (SDK stubs, no GPU dispatch)");

    return 0;
}
