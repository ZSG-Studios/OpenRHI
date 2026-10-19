// Production NRC adapter exercised through deterministic public SDK implementations
#define EXPORTING_NRC
#include <d3d12.h>
#include <malloc.h>
#include <cstdio>
#include <cstdlib>
#include "NrcD3d12.h"
#include "SharedExternal.h"
#include "VendorFeatures.h"

using namespace openrhi;
#include "VendorFeatures.hpp"

struct MockContext : nrc::d3d12::Context {};

static MockContext nativeContexts[8];
static nrc::d3d12::Buffers nativeBuffers;
static uint32_t initialized, shutdowns, created, destroyed, configured, begins, queries, resolves, ends, allocations;
static uint32_t invalid;
static nrc::Status failInit = nrc::Status::OK, failCreate = nrc::Status::OK, failConfigure = nrc::Status::OK, failBegin = nrc::Status::OK, failEnd = nrc::Status::OK;
static bool partialCreate;
static bool sdkInitialized;
static ID3D12GraphicsCommandList4* commandToken = (ID3D12GraphicsCommandList4*)0x123400;
static ID3D12CommandQueue* queueToken = (ID3D12CommandQueue*)0x234500;

nrc::Status nrc::d3d12::Initialize(const GlobalSettings&) {
    ++initialized;
    sdkInitialized = failInit == nrc::Status::OK;
    return failInit;
}

void nrc::d3d12::Shutdown() {
    ++shutdowns;
    sdkInitialized = false;
}

nrc::Status nrc::d3d12::Context::Create(ID3D12Device5*, Context*& out) {
    out = (failCreate == Status::OK || partialCreate) ? &nativeContexts[(created++) % 8] : nullptr;
    return failCreate;
}

nrc::Status nrc::d3d12::Context::Destroy(Context&) {
    ++destroyed;
    return Status::OK;
}

nrc::Status nrc::d3d12::Context::Configure(const ContextSettings& settings, const Buffers*, const char*) {
    ++configured;
    invalid += settings.frameDimensions.x != 64;
    return failConfigure;
}

nrc::Status nrc::d3d12::Context::GetBuffersAllocationInfo(const ContextSettings&, BuffersAllocationInfo& out) {
    out = {};
    return sdkInitialized ? Status::OK : Status::SDKNotInitialized;
}

nrc::Status nrc::d3d12::Context::PopulateShaderConstants(NrcConstants& out) const {
    out = {};
    out.frameDimensions = {64, 64};
    return Status::OK;
}

nrc::Status nrc::d3d12::Context::BeginFrame(ID3D12GraphicsCommandList4* command, const FrameSettings&) {
    ++begins;
    invalid += command != commandToken;
    return failBegin;
}

nrc::Status nrc::d3d12::Context::QueryAndTrain(ID3D12GraphicsCommandList4* command, float* loss) {
    ++queries;
    invalid += command != commandToken;
    if (loss)
        *loss = 0.25f;
    return Status::OK;
}

nrc::Status nrc::d3d12::Context::Resolve(ID3D12GraphicsCommandList4* command, ID3D12Resource* output) {
    ++resolves;
    invalid += command != commandToken || output != (ID3D12Resource*)0x345600;
    return Status::OK;
}

nrc::Status nrc::d3d12::Context::EndFrame(ID3D12CommandQueue* queue) {
    ++ends;
    invalid += queue != queueToken;
    return failEnd;
}

D3D12_RESOURCE_STATES nrc::d3d12::Context::GetInitialBufferState(const AllocationInfo&) const {
    return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

const nrc::d3d12::Buffers* nrc::d3d12::Context::GetBuffers() const {
    return &nativeBuffers;
}

Library* openrhi::LoadSharedLibrary(const char*) {
    return (Library*)0x5000;
}

void openrhi::UnloadSharedLibrary(Library&) {
}

struct TestDevice : DeviceBase {
    TestDevice(const AllocationCallbacks& alloc) : DeviceBase({}, alloc) {
        desc.graphicsAPI = GraphicsAPI::D3D12;
        desc.adapterDesc.vendor = Vendor::NVIDIA;
    }

    const DeviceDesc& GetDesc() const override {
        return desc;
    }

    void Destruct() override {
    }

    DeviceDesc desc = {};
};

static void* OPENRHI_CALL Alloc(void*, size_t size, size_t alignment) {
    ++allocations;
    return _aligned_malloc(size, alignment);
}

static void OPENRHI_CALL Free(void*, void* memory) {
    _aligned_free(memory);
}

static const DeviceDesc& OPENRHI_CALL Desc(const Device& device) {
    return ((const TestDevice&)device).GetDesc();
}

static void* OPENRHI_CALL DeviceNative(const Device*) {
    return (void*)0x1000;
}

static void* OPENRHI_CALL CommandNative(const CommandBuffer*) {
    return commandToken;
}

static void* OPENRHI_CALL QueueNative(const Queue*) {
    return queueToken;
}

Result OPENRHI_CALL openrhi::openrhiGetInterface(const Device&, const char*, size_t size, void* output) {
    if (size != sizeof(CoreInterface))
        return Result::UNSUPPORTED;
    auto& core = *(CoreInterface*)output;
    core = {};
    core.GetDeviceDesc = Desc;
    core.GetDeviceNativeObject = DeviceNative;
    core.GetCommandBufferNativeObject = CommandNative;
    core.GetQueueNativeObject = QueueNative;
    return Result::SUCCESS;
}

#define CHECK(x) \
    do { \
        if (!(x)) { \
            std::printf("FAIL line %d: %s\n", __LINE__, #x); \
            return 1; \
        } \
    } while (false)

int main() {
    const AllocationCallbacks allocator = {Alloc, nullptr, Free};
    TestDevice device(allocator);
    nrc::GlobalSettings global;
    nrc::ContextSettings settings;
    settings.frameDimensions = {64, 64};
    NeuralRadianceCacheDesc desc = {&global, &settings, nullptr, nullptr};
    NeuralRadianceCacheInterface table = {};
    CHECK(FillVendorFeaturesInterface(table) == Result::SUCCESS);
    NeuralRadianceCache* cache = (NeuralRadianceCache*)1;
    failInit = nrc::Status::UnsupportedDriver;
    CHECK(table.CreateNeuralRadianceCache((Device&)device, desc, cache) == Result::UNSUPPORTED && !cache && !shutdowns);
    failInit = nrc::Status::OK;
    failCreate = nrc::Status::OutOfMemory;
    partialCreate = true;
    CHECK(table.CreateNeuralRadianceCache((Device&)device, desc, cache) == Result::OUT_OF_MEMORY && !cache && destroyed == 1 && shutdowns == 1);
    failCreate = nrc::Status::OK;
    partialCreate = false;
    failConfigure = nrc::Status::WrongParameter;
    CHECK(table.CreateNeuralRadianceCache((Device&)device, desc, cache) == Result::INVALID_ARGUMENT && !cache && shutdowns == 2);
    failConfigure = nrc::Status::OK;
    CHECK(table.CreateNeuralRadianceCache((Device&)device, desc, cache) == Result::SUCCESS && cache);
    const auto initCount = initialized;
    NeuralRadianceCache* second = nullptr;
    CHECK(table.CreateNeuralRadianceCache((Device&)device, desc, second) == Result::SUCCESS && initialized == initCount);
    nrc::GlobalSettings incompatible = global;
    incompatible.maxNumFramesInFlight++;
    NeuralRadianceCacheDesc other = desc;
    other.globalSettings = &incompatible;
    NeuralRadianceCache* rejected = (NeuralRadianceCache*)1;
    CHECK(table.CreateNeuralRadianceCache((Device&)device, other, rejected) == Result::INVALID_ARGUMENT && !rejected);
    table.DestroyNeuralRadianceCache(second);
    CHECK(shutdowns == 2);
    nrc::FrameSettings frame;
    uint64_t commandStorage = 0, queueStorage = 0;
    auto& command = *(CommandBuffer*)&commandStorage;
    auto& queue = *(Queue*)&queueStorage;
    CHECK(table.CmdQueryAndTrainNeuralRadianceCache(command, *cache, nullptr) == Result::INVALID_ARGUMENT);
    failBegin = nrc::Status::InternalError;
    CHECK(table.CmdBeginNeuralRadianceCacheFrame(command, *cache, &frame) == Result::FAILURE);
    failBegin = nrc::Status::OK;
    const auto allocationCount = allocations;
    for (uint32_t i = 0; i < 3; ++i) {
        CHECK(table.CmdBeginNeuralRadianceCacheFrame(command, *cache, &frame) == Result::SUCCESS);
        CHECK(table.CmdBeginNeuralRadianceCacheFrame(command, *cache, &frame) == Result::INVALID_ARGUMENT);
        CHECK(table.ConfigureNeuralRadianceCache(*cache, &settings, nullptr, nullptr) == Result::INVALID_ARGUMENT);
        CHECK(table.GetNeuralRadianceCacheBuffers(*cache) == &nativeBuffers);
        NrcConstants constants = {};
        CHECK(table.GetNeuralRadianceCacheShaderConstants(*cache, &constants) == Result::SUCCESS && constants.frameDimensions.x == 64);
        float loss = 0;
        CHECK(table.CmdQueryAndTrainNeuralRadianceCache(command, *cache, &loss) == Result::SUCCESS && loss == 0.25f);
        CHECK(table.CmdResolveNeuralRadianceCache(command, *cache, 0x345600) == Result::SUCCESS);
        failEnd = nrc::Status::InternalError;
        CHECK(table.EndNeuralRadianceCacheFrame(queue, *cache) == Result::FAILURE);
        failEnd = nrc::Status::OK;
        CHECK(table.EndNeuralRadianceCacheFrame(queue, *cache) == Result::SUCCESS);
        CHECK(table.EndNeuralRadianceCacheFrame(queue, *cache) == Result::INVALID_ARGUMENT);
    }
    CHECK(allocations == allocationCount && !invalid && queries == 3 && resolves == 3);
    table.DestroyNeuralRadianceCache(cache);
    table.DestroyNeuralRadianceCache(nullptr);
    CHECK(shutdowns == 3);
    nrc::BuffersAllocationInfo allocationInfo;
    CHECK(table.GetNeuralRadianceCacheAllocationInfo((Device&)device, desc, &allocationInfo) == Result::SUCCESS);
    CHECK(shutdowns == 4);
    std::puts("NRC production adapter contract: PASS (SDK stubs, lifecycle/rollback/shared initialization/native handles/frame ordering/allocation-free calls)");
    return 0;
}
