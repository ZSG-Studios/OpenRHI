#include "VendorFeaturesVal.h"

// Native descriptor contents follow the pinned provider contract and its native validation
// OpenRHI validates its public pointers, effect selection and command recording state
template <typename T>
static inline T GetVendorTable() {
    T table = {};
    FillVendorFeaturesInterface(table);

    return table;
}

template <typename T, typename DestroyFn>
static inline Result WrapVendorContext(DeviceVal& device, Result result, T* impl, T*& output, DestroyFn destroy) {
    if (result != Result::SUCCESS)
        return result;

    auto* val = Allocate<VendorContextVal<T>>(device.GetAllocationCallbacks(), device, impl);

    if (!val) {
        destroy(impl);

        return Result::OUT_OF_MEMORY;
    }
    output = (T*)val;

    return Result::SUCCESS;
}

static Result OPENRHI_CALL CreateFidelityFXContextVal(Device& device, const FidelityFXDesc& desc, FidelityFXContext*& context) {
    auto& deviceVal = (DeviceVal&)device;
    context = nullptr;
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, desc.effect < FidelityFXEffect::MAX_NUM, Result::INVALID_ARGUMENT, "Invalid FidelityFX effect or native description");

    const auto table = GetVendorTable<FidelityFXInterface>();
    FidelityFXContext* impl = nullptr;
    Result result = table.CreateFidelityFXContext(device, desc, impl);

    return WrapVendorContext(deviceVal, result, impl, context, table.DestroyFidelityFXContext);
}

static Result OPENRHI_CALL ConfigureFidelityFXVal(FidelityFXContext& context, const void* desc) {
#if OPENRHI_ENABLE_FFX_SDK && OPENRHI_ENABLE_D3D12_SUPPORT
    auto& deviceVal = ((VendorContextVal<FidelityFXContext>&)context).GetDevice();
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, desc, Result::INVALID_ARGUMENT, "Native description is null");
#endif

    return GetVendorTable<FidelityFXInterface>().ConfigureFidelityFX(*((VendorContextVal<FidelityFXContext>&)context).GetImpl(), desc);
}

static Result OPENRHI_CALL QueryFidelityFXVal(FidelityFXContext& context, void* desc) {
#if OPENRHI_ENABLE_FFX_SDK && OPENRHI_ENABLE_D3D12_SUPPORT
    auto& deviceVal = ((VendorContextVal<FidelityFXContext>&)context).GetDevice();
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, desc, Result::INVALID_ARGUMENT, "Native description is null");
#endif

    return GetVendorTable<FidelityFXInterface>().QueryFidelityFX(*((VendorContextVal<FidelityFXContext>&)context).GetImpl(), desc);
}

static Result OPENRHI_CALL CmdDispatchFidelityFXVal(CommandBuffer& commandBuffer, FidelityFXContext& context, const void* desc) {
    const auto& commandVal = (CommandBufferVal&)commandBuffer;
    auto& deviceVal = commandVal.GetDevice();
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, desc && commandVal.CanDispatch(), Result::INVALID_ARGUMENT, "Dispatch requires a native description and an open compute-capable command buffer outside rendering");

    return GetVendorTable<FidelityFXInterface>().CmdDispatchFidelityFX(commandBuffer, *((VendorContextVal<FidelityFXContext>&)context).GetImpl(), desc);
}

static void OPENRHI_CALL DestroyFidelityFXContextVal(FidelityFXContext* context) {
    if (!context)
        return;

    auto* val = (VendorContextVal<FidelityFXContext>*)context;
    GetVendorTable<FidelityFXInterface>().DestroyFidelityFXContext(val->GetImpl());
    Destroy(val);
}

Result DeviceVal::FillFunctionTable(FidelityFXInterface& table) const {
    Result result = FillVendorFeaturesInterface(table);

    if (result == Result::SUCCESS) {
        table.CreateFidelityFXContext = ::CreateFidelityFXContextVal;
        table.DestroyFidelityFXContext = ::DestroyFidelityFXContextVal;
        table.ConfigureFidelityFX = ::ConfigureFidelityFXVal;
        table.QueryFidelityFX = ::QueryFidelityFXVal;
        table.CmdDispatchFidelityFX = ::CmdDispatchFidelityFXVal;
    }

    return result;
}

static Result OPENRHI_CALL CreateNeuralRadianceCacheVal(Device& device, const NeuralRadianceCacheDesc& desc, NeuralRadianceCache*& cache) {
    auto& deviceVal = (DeviceVal&)device;
    cache = nullptr;
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, desc.globalSettings && desc.contextSettings, Result::INVALID_ARGUMENT, "NRC settings are null");

    const auto table = GetVendorTable<NeuralRadianceCacheInterface>();
    NeuralRadianceCache* impl = nullptr;
    Result result = table.CreateNeuralRadianceCache(device, desc, impl);

    return WrapVendorContext(deviceVal, result, impl, cache, table.DestroyNeuralRadianceCache);
}

static Result OPENRHI_CALL ConfigureNeuralRadianceCacheVal(NeuralRadianceCache& cache, const void* settings, const void* buffers, const char* filename) {
#if OPENRHI_ENABLE_NRC_SDK
    auto& deviceVal = ((VendorContextVal<NeuralRadianceCache>&)cache).GetDevice();
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, settings, Result::INVALID_ARGUMENT, "NRC settings are null");
#endif

    return GetVendorTable<NeuralRadianceCacheInterface>().ConfigureNeuralRadianceCache(*((VendorContextVal<NeuralRadianceCache>&)cache).GetImpl(), settings, buffers, filename);
}

static Result OPENRHI_CALL GetNeuralRadianceCacheAllocationInfoVal(const Device& device, const NeuralRadianceCacheDesc& desc, void* info) {
    const auto& deviceVal = (const DeviceVal&)device;
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, desc.globalSettings && desc.contextSettings && info, Result::INVALID_ARGUMENT, "NRC settings or allocation output are null");

    return GetVendorTable<NeuralRadianceCacheInterface>().GetNeuralRadianceCacheAllocationInfo(device, desc, info);
}

static Result OPENRHI_CALL GetNeuralRadianceCacheInitialBufferStateVal(const NeuralRadianceCache& cache, const void* allocation, uint32_t& state) {
#if OPENRHI_ENABLE_NRC_SDK
    auto& deviceVal = ((const VendorContextVal<NeuralRadianceCache>&)cache).GetDevice();
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, allocation, Result::INVALID_ARGUMENT, "NRC allocation info is null");
#endif

    return GetVendorTable<NeuralRadianceCacheInterface>().GetNeuralRadianceCacheInitialBufferState(*((const VendorContextVal<NeuralRadianceCache>&)cache).GetImpl(), allocation, state);
}

static Result OPENRHI_CALL GetNeuralRadianceCacheShaderConstantsVal(const NeuralRadianceCache& cache, void* constants) {
#if OPENRHI_ENABLE_NRC_SDK
    auto& deviceVal = ((const VendorContextVal<NeuralRadianceCache>&)cache).GetDevice();
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, constants, Result::INVALID_ARGUMENT, "NRC constants output is null");
#endif

    return GetVendorTable<NeuralRadianceCacheInterface>().GetNeuralRadianceCacheShaderConstants(*((const VendorContextVal<NeuralRadianceCache>&)cache).GetImpl(), constants);
}

static Result OPENRHI_CALL CmdBeginNeuralRadianceCacheFrameVal(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, const void* settings) {
    const auto& commandVal = (CommandBufferVal&)commandBuffer;
    auto& deviceVal = commandVal.GetDevice();
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, settings && commandVal.CanDispatch(), Result::INVALID_ARGUMENT, "NRC begin requires frame settings and an open compute-capable command buffer outside rendering");

    return GetVendorTable<NeuralRadianceCacheInterface>().CmdBeginNeuralRadianceCacheFrame(commandBuffer, *((VendorContextVal<NeuralRadianceCache>&)cache).GetImpl(), settings);
}

static Result OPENRHI_CALL CmdQueryAndTrainNeuralRadianceCacheVal(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, float* loss) {
    const auto& commandVal = (CommandBufferVal&)commandBuffer;
    auto& deviceVal = commandVal.GetDevice();
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, commandVal.CanDispatch(), Result::INVALID_ARGUMENT, "NRC query/train requires an open compute-capable command buffer outside rendering");

    return GetVendorTable<NeuralRadianceCacheInterface>().CmdQueryAndTrainNeuralRadianceCache(commandBuffer, *((VendorContextVal<NeuralRadianceCache>&)cache).GetImpl(), loss);
}

static Result OPENRHI_CALL CmdResolveNeuralRadianceCacheVal(CommandBuffer& commandBuffer, NeuralRadianceCache& cache, uint64_t output) {
    const auto& commandVal = (CommandBufferVal&)commandBuffer;
    auto& deviceVal = commandVal.GetDevice();
    OPENRHI_RETURN_ON_FAILURE(&deviceVal, output && commandVal.CanDispatch(), Result::INVALID_ARGUMENT, "NRC resolve requires an output and an open compute-capable command buffer outside rendering");

    return GetVendorTable<NeuralRadianceCacheInterface>().CmdResolveNeuralRadianceCache(commandBuffer, *((VendorContextVal<NeuralRadianceCache>&)cache).GetImpl(), output);
}

static void OPENRHI_CALL DestroyNeuralRadianceCacheVal(NeuralRadianceCache* cache) {
    if (!cache)
        return;

    auto* val = (VendorContextVal<NeuralRadianceCache>*)cache;
    GetVendorTable<NeuralRadianceCacheInterface>().DestroyNeuralRadianceCache(val->GetImpl());
    Destroy(val);
}

static const void* OPENRHI_CALL GetNeuralRadianceCacheBuffersVal(const NeuralRadianceCache& cache) {
    return GetVendorTable<NeuralRadianceCacheInterface>().GetNeuralRadianceCacheBuffers(*((const VendorContextVal<NeuralRadianceCache>&)cache).GetImpl());
}

static Result OPENRHI_CALL EndNeuralRadianceCacheFrameVal(Queue& queue, NeuralRadianceCache& cache) {
    return GetVendorTable<NeuralRadianceCacheInterface>().EndNeuralRadianceCacheFrame(queue, *((VendorContextVal<NeuralRadianceCache>&)cache).GetImpl());
}

Result DeviceVal::FillFunctionTable(NeuralRadianceCacheInterface& table) const {
    Result result = FillVendorFeaturesInterface(table);

    if (result == Result::SUCCESS) {
        table.CreateNeuralRadianceCache = ::CreateNeuralRadianceCacheVal;
        table.DestroyNeuralRadianceCache = ::DestroyNeuralRadianceCacheVal;
        table.GetNeuralRadianceCacheBuffers = ::GetNeuralRadianceCacheBuffersVal;
        table.EndNeuralRadianceCacheFrame = ::EndNeuralRadianceCacheFrameVal;
        table.ConfigureNeuralRadianceCache = ::ConfigureNeuralRadianceCacheVal;
        table.GetNeuralRadianceCacheAllocationInfo = ::GetNeuralRadianceCacheAllocationInfoVal;
        table.GetNeuralRadianceCacheInitialBufferState = ::GetNeuralRadianceCacheInitialBufferStateVal;
        table.GetNeuralRadianceCacheShaderConstants = ::GetNeuralRadianceCacheShaderConstantsVal;
        table.CmdBeginNeuralRadianceCacheFrame = ::CmdBeginNeuralRadianceCacheFrameVal;
        table.CmdQueryAndTrainNeuralRadianceCache = ::CmdQueryAndTrainNeuralRadianceCacheVal;
        table.CmdResolveNeuralRadianceCache = ::CmdResolveNeuralRadianceCacheVal;
    }

    return result;
}

static Result OPENRHI_CALL CreateAntiLagContextVal(Device& device, AntiLagContext*& context) {
    auto& deviceVal = (DeviceVal&)device;
    context = nullptr;
    AntiLagContext* impl = nullptr;
    const auto table = GetVendorTable<AntiLagInterface>();
    Result result = table.CreateAntiLagContext(device, impl);

    return WrapVendorContext(deviceVal, result, impl, context, table.DestroyAntiLagContext);
}

static void OPENRHI_CALL DestroyAntiLagContextVal(AntiLagContext* context) {
    if (!context)
        return;

    auto* val = (VendorContextVal<AntiLagContext>*)context;
    GetVendorTable<AntiLagInterface>().DestroyAntiLagContext(val->GetImpl());
    Destroy(val);
}

static Result OPENRHI_CALL UpdateAntiLagVal(AntiLagContext& context, bool enabled, uint32_t maxFPS) {
    return GetVendorTable<AntiLagInterface>().UpdateAntiLag(*((VendorContextVal<AntiLagContext>&)context).GetImpl(), enabled, maxFPS);
}

static Result OPENRHI_CALL MarkAntiLagEndOfRenderingVal(AntiLagContext& context) {
    return GetVendorTable<AntiLagInterface>().MarkAntiLagEndOfRendering(*((VendorContextVal<AntiLagContext>&)context).GetImpl());
}

static Result OPENRHI_CALL SetAntiLagFrameTypeVal(AntiLagContext& context, bool interpolated) {
    return GetVendorTable<AntiLagInterface>().SetAntiLagFrameType(*((VendorContextVal<AntiLagContext>&)context).GetImpl(), interpolated);
}

static Result OPENRHI_CALL AttachAntiLagSwapChainVal(AntiLagContext& context, void* swapChain) {
    return GetVendorTable<AntiLagInterface>().AttachAntiLagSwapChain(*((VendorContextVal<AntiLagContext>&)context).GetImpl(), swapChain);
}

Result DeviceVal::FillFunctionTable(AntiLagInterface& table) const {
    Result result = FillVendorFeaturesInterface(table);

    if (result == Result::SUCCESS)
        table = {::CreateAntiLagContextVal, ::DestroyAntiLagContextVal, ::UpdateAntiLagVal, ::MarkAntiLagEndOfRenderingVal, ::SetAntiLagFrameTypeVal, ::AttachAntiLagSwapChainVal};

    return result;
}
