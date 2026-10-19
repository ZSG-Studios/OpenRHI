#pragma once

namespace openrhi {

template <typename T>
struct VendorContextVal final : public ObjectVal {
    inline VendorContextVal(DeviceVal& device, T* impl) : ObjectVal(device, impl) {
    }

    inline T* GetImpl() const {
        return (T*)m_Impl;
    }
};

} // namespace openrhi
