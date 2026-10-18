// © 2026 NVIDIA Corporation

#pragma once

#include "InternalShadersMetal.h"

namespace openrhi {

struct DeviceMetal final : public DeviceBase {
    DeviceMetal(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks);
    ~DeviceMetal();

    inline MTL::Device* GetNativeObject() const {
        return m_Device;
    }

    inline MTL4::Compiler* GetCompiler() const {
        return m_Compiler;
    }

    inline const DeviceDesc& GetDesc() const override {
        return m_Desc;
    }

    Result Create(const DeviceCreationDesc& desc, const DeviceCreationMetalDesc& metalDesc);
    void Destruct() override;
    void AddResidency(MTL::Allocation* allocation);
    void RemoveResidency(MTL::Allocation* allocation);
    void CommitResidency();                                    // no-op if the residency set hasn't changed
    bool IsNativeAverageResolveSupported(Format format) const; // otherwise "AVERAGE" is resolved by a shader
    void AddQueueResidencySet(MTL::ResidencySet* residencySet);
    void RemoveQueueResidencySet(MTL::ResidencySet* residencySet);

    inline InternalShadersMetal& GetInternalShaders() {
        return m_InternalShaders;
    }

    MTL::ResidencySet* GetResidencySet() const {
        return m_Residency;
    }

    Result GetQueue(QueueType type, uint32_t index, Queue*& queue);
    Result CreatePlacedBuffer(Memory* memory, uint64_t offset, const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreatePlacedTexture(Memory* memory, uint64_t offset, const TextureDesc& textureDesc, Texture*& texture);
    Result CreatePlacedAccelerationStructure(Memory* memory, uint64_t offset, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result WaitIdle();
    FormatSupportBits GetFormatSupport(Format format) const;
    bool IsUpscalerSupported(UpscalerType upscalerType) const;
    void GetMemoryDesc2(const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc);
    void GetMemoryDesc2(const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc);
    void GetMemoryDesc2(const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc);
    Result BindBufferMemory(const BindBufferMemoryDesc* bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum);
    Result BindTextureMemory(const BindTextureMemoryDesc* bindTextureMemoryDescs, uint32_t bindTextureMemoryDescNum);
    Result BindAccelerationStructureMemory(const BindAccelerationStructureMemoryDesc* bindAccelerationStructureMemoryDescs, uint32_t bindAccelerationStructureMemoryDescNum);
    Result QueryVideoMemoryInfo(MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) const;

    const CoreInterface& GetCoreInterface() const {
        return m_Core;
    }

    Result FillFunctionTable(CoreInterface& table) const override;
    Result FillFunctionTable(HelperInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(MeshShaderInterface& table) const override;
    Result FillFunctionTable(RayTracingInterface& table) const override;
    Result FillFunctionTable(DescriptorHeapInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;
    Result FillFunctionTable(WrapperMetalInterface& table) const override;
#if OPENRHI_ENABLE_IMGUI_EXTENSION
    Result FillFunctionTable(ImguiInterface& table) const override;
#endif

    template <typename Implementation, typename Interface, typename... Args>
    Result CreateImplementation(Interface*& entity, const Args&... args) {
        auto* impl = Allocate<Implementation>(GetAllocationCallbacks(), *this);
        entity = nullptr;

        if (!impl)
            return Result::OUT_OF_MEMORY;

        Result result = impl->Create(args...);

        if (result != Result::SUCCESS)
            Destroy(GetAllocationCallbacks(), impl);
        else
            entity = (Interface*)impl;

        return result;
    }

private:
    void FillDesc(const AdapterDesc& adapterDesc);

    DeviceDesc m_Desc = {};
    MTL::Device* m_Device = nullptr;
    MTL::ResidencySet* m_Residency = nullptr;
    MTL4::Compiler* m_Compiler = nullptr;
    QueueMetal* m_Queues[QUEUE_TYPE_NUM][QUEUE_NUM_PER_TYPE] = {};
    std::mutex m_ResidencyLock;
    UnorderedMap<MTL::Allocation*, uint32_t> m_ResidencyReferences;
    std::atomic_bool m_IsResidencyDirty = false; // written under "m_ResidencyLock"
    CoreInterface m_Core = {};
    InternalShadersMetal m_InternalShaders;
};

} // namespace openrhi
