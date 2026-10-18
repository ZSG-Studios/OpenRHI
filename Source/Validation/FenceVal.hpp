// © 2021 NVIDIA Corporation

OPENRHI_INLINE uint64_t FenceVal::GetFenceValue() const {
    return GetCoreInterfaceImpl().GetFenceValue(*GetImpl());
}

OPENRHI_INLINE void FenceVal::Wait(uint64_t value) {
    GetCoreInterfaceImpl().Wait(*GetImpl(), value);
}
