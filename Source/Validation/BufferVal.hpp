// © 2021 NVIDIA Corporation

BufferVal::~BufferVal() {
    if (m_Memory)
        m_Memory->Unbind(*this);
}

OPENRHI_INLINE void* BufferVal::Map(uint64_t offset, uint64_t size) {
    if (size == WHOLE_SIZE)
        size = m_Desc.size;

    OPENRHI_RETURN_ON_FAILURE(&m_Device, m_IsBoundToMemory, nullptr, "the buffer is not bound to memory");
    OPENRHI_RETURN_ON_FAILURE(&m_Device, !m_IsMapped, nullptr, "the buffer is already mapped");
    OPENRHI_RETURN_ON_FAILURE(&m_Device, offset + size <= m_Desc.size, nullptr, "out of bounds");

    m_IsMapped = true;

    return GetCoreInterfaceImpl().MapBuffer(*GetImpl(), offset, size);
}

OPENRHI_INLINE void BufferVal::Unmap() {
    OPENRHI_RETURN_ON_FAILURE(&m_Device, m_IsMapped, ReturnVoid(), "the buffer is not mapped");

    m_IsMapped = false;

    GetCoreInterfaceImpl().UnmapBuffer(*GetImpl());
}

OPENRHI_INLINE uint64_t BufferVal::GetDeviceAddress() const {
    return GetCoreInterfaceImpl().GetBufferDeviceAddress(*GetImpl());
}
