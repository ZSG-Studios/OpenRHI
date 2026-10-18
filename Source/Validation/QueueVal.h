// © 2021 NVIDIA Corporation

#pragma once

namespace openrhi {

struct QueueVal final : public ObjectVal {
    inline QueueVal(DeviceVal& device, Queue* queue, QueueType queueType)
        : ObjectVal(device, queue)
        , m_QueueType(queueType) {
    }

    inline Queue* GetImpl() const {
        return (Queue*)m_Impl;
    }

    inline QueueType GetType() const {
        return m_QueueType;
    }

    inline void* GetNativeObject() const {
        return m_Device.GetCoreInterfaceImpl().GetQueueNativeObject(GetImpl());
    }

    //================================================================================================================
    // OpenRHI
    //================================================================================================================

    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);
    void GetCalibratedTimestamps(uint64_t& timestampGPU, uint64_t& timestampCPU);
    Result Submit(const QueueSubmitDesc& queueSubmitDesc);
    Result WaitIdle();

private:
    QueueType m_QueueType = QueueType::MAX_NUM;
};

} // namespace openrhi
