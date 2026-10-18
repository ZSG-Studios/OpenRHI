// © 2021 NVIDIA Corporation

MicromapVal::~MicromapVal() {
    if (m_Memory)
        m_Memory->Unbind(*this);

    Destroy(m_Buffer);
}

OPENRHI_INLINE uint64_t MicromapVal::GetBuildScratchBufferSize() const {
    return GetRayTracingInterfaceImpl().GetMicromapBuildScratchBufferSize(*GetImpl());
}

OPENRHI_INLINE uint64_t MicromapVal::GetNativeObject() const {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "Micromap is not bound to memory");

    return GetRayTracingInterfaceImpl().GetMicromapNativeObject(GetImpl());
}

OPENRHI_INLINE Buffer* MicromapVal::GetBuffer() {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "Micromap is not bound to memory");

    if (!m_Buffer) {
        Buffer* buffer = GetRayTracingInterfaceImpl().GetMicromapBuffer(*GetImpl());
        m_Buffer = Allocate<BufferVal>(m_Device.GetAllocationCallbacks(), m_Device, buffer, false);
    }

    return (Buffer*)m_Buffer;
}
