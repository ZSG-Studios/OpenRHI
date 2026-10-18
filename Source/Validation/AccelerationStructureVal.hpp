// © 2021 NVIDIA Corporation

AccelerationStructureVal::~AccelerationStructureVal() {
    if (m_Memory)
        m_Memory->Unbind(*this);

    Destroy(m_Buffer);
}

OPENRHI_INLINE uint64_t AccelerationStructureVal::GetUpdateScratchBufferSize() const {
    return GetRayTracingInterfaceImpl().GetAccelerationStructureUpdateScratchBufferSize(*GetImpl());
}

OPENRHI_INLINE uint64_t AccelerationStructureVal::GetBuildScratchBufferSize() const {
    return GetRayTracingInterfaceImpl().GetAccelerationStructureBuildScratchBufferSize(*GetImpl());
}

OPENRHI_INLINE uint64_t AccelerationStructureVal::GetHandle() const {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "AccelerationStructure is not bound to memory");

    return GetRayTracingInterfaceImpl().GetAccelerationStructureHandle(*GetImpl());
}

OPENRHI_INLINE uint64_t AccelerationStructureVal::GetNativeObject() const {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "AccelerationStructure is not bound to memory");

    return GetRayTracingInterfaceImpl().GetAccelerationStructureNativeObject(GetImpl());
}

OPENRHI_INLINE Buffer* AccelerationStructureVal::GetBuffer() {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "AccelerationStructure is not bound to memory");

    if (!m_Buffer) {
        Buffer* buffer = GetRayTracingInterfaceImpl().GetAccelerationStructureBuffer(*GetImpl());
        m_Buffer = Allocate<BufferVal>(m_Device.GetAllocationCallbacks(), m_Device, buffer, false);
    }

    return (Buffer*)m_Buffer;
}

OPENRHI_INLINE Result AccelerationStructureVal::CreateDescriptor(Descriptor*& descriptor) {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), Result::INVALID_ARGUMENT, "AccelerationStructure is not bound to memory");

    Descriptor* descriptorImpl = nullptr;
    const Result result = GetRayTracingInterfaceImpl().CreateAccelerationStructureDescriptor(*GetImpl(), descriptorImpl);

    descriptor = nullptr;
    if (result == Result::SUCCESS)
        descriptor = (Descriptor*)Allocate<DescriptorVal>(m_Device.GetAllocationCallbacks(), m_Device, descriptorImpl, DescriptorType::ACCELERATION_STRUCTURE);

    return result;
}
