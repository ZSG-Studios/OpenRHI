// © 2026 NVIDIA Corporation

#pragma once

namespace openrhi {

struct VideoSessionVK;

struct VideoSessionParametersVK final : public DebugNameBase {
    inline VideoSessionParametersVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline const VideoSessionVK& GetSession() const {
        return *m_Session;
    }

    inline const StdVideoAV1SequenceHeader& GetAV1SequenceHeader() const {
        return m_AV1SequenceHeader;
    }

    inline VkVideoSessionParametersKHR GetHandle() const {
        return m_Handle;
    }

    ~VideoSessionParametersVK();

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) OPENRHI_DEBUG_NAME_OVERRIDE {
        m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR, (uint64_t)m_Handle, name);
    }

    //================================================================================================================
    // OpenRHI
    //================================================================================================================

    Result Create(const VideoSessionParametersDesc& videoSessionParametersDesc);

private:
    Result CreateNative(VideoSessionVK& session, const void* pNext);
    Result CreateH265(VideoSessionVK& session, const VideoH265SessionParametersDesc* parameters);
    Result CreateAV1(VideoSessionVK& session, const VideoAV1SessionParametersDesc* parameters);

    DeviceVK& m_Device;
    VideoSessionVK* m_Session = nullptr;
    VkVideoSessionParametersKHR m_Handle = VK_NULL_HANDLE;
    StdVideoAV1ColorConfig m_AV1ColorConfig = {};
    StdVideoAV1TimingInfo m_AV1TimingInfo = {};
    StdVideoAV1SequenceHeader m_AV1SequenceHeader = {};
    StdVideoEncodeAV1DecoderModelInfo m_AV1DecoderModelInfo = {};
    StdVideoEncodeAV1OperatingPointInfo m_AV1OperatingPoint = {};
};

} // namespace openrhi
