// © 2026 NVIDIA Corporation

OPENRHI_INLINE VideoSessionVal::VideoSessionVal(DeviceVal& device, VideoSession* impl, const VideoSessionDesc& desc, const VideoCapabilities& capabilities)
    : ObjectVal(device, (Object*)impl)
    , m_Desc(desc)
    , m_Capabilities(capabilities) {
}

OPENRHI_INLINE VideoSession* VideoSessionVal::GetImpl() const {
    return (VideoSession*)m_Impl;
}

OPENRHI_INLINE const VideoSessionDesc& VideoSessionVal::GetDesc() const {
    return m_Desc;
}

OPENRHI_INLINE const VideoCapabilities& VideoSessionVal::GetCapabilities() const {
    return m_Capabilities;
}

OPENRHI_INLINE bool VideoSessionVal::IsResolvedMetadataRangeValid(const BufferVal& buffer, uint64_t offset) const {
    const BufferDesc& bufferDesc = buffer.GetDesc();
    const bool isOffsetAligned = m_Capabilities.resolvedMetadataOffsetAlignment <= 1 || offset % m_Capabilities.resolvedMetadataOffsetAlignment == 0;
    const bool isRangeValid = offset <= bufferDesc.size && m_Capabilities.resolvedMetadataSize <= bufferDesc.size - offset;

    return isOffsetAligned && isRangeValid;
}
