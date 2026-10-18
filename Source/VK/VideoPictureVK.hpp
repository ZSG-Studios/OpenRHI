// © 2026 NVIDIA Corporation

VideoPictureVK::~VideoPictureVK() {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_ImageView)
        vk.DestroyImageView(m_Device, m_ImageView, m_Device.GetVkAllocationCallbacks());
}

OPENRHI_INLINE Result VideoPictureVK::Create(const VideoPictureDesc& videoPictureDesc) {
    TextureVK& texture = *(TextureVK*)videoPictureDesc.texture;
    const TextureDesc& textureDesc = texture.GetDesc();
    m_Texture = &texture;
    m_Layer = videoPictureDesc.layer;

    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    createInfo.image = texture.GetHandle();
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = GetVkFormat(textureDesc.format);
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = videoPictureDesc.layer;
    createInfo.subresourceRange.layerCount = 1;

    switch (videoPictureDesc.usage) {
        case VideoPictureUsage::DECODE_OUTPUT:
            usageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR | VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;
            break;
        case VideoPictureUsage::DECODE_REFERENCE:
            usageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;
            break;
        case VideoPictureUsage::ENCODE_INPUT:
            usageInfo.usage |= VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR;
            break;
        case VideoPictureUsage::ENCODE_REFERENCE:
            usageInfo.usage |= VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR;
            break;
        default:
            OPENRHI_CHECK(false, "Unexpected 'videoPictureDesc.usage'");
            break;
    }

    if (usageInfo.usage)
        createInfo.pNext = &usageInfo;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateImageView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_ImageView);
    OPENRHI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateImageView");

    m_Resource = {VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR};
    m_Resource.codedExtent.width = videoPictureDesc.width ? videoPictureDesc.width : textureDesc.width;
    m_Resource.codedExtent.height = videoPictureDesc.height ? videoPictureDesc.height : textureDesc.height;
    m_Resource.baseArrayLayer = 0;
    m_Resource.imageViewBinding = m_ImageView;

    return Result::SUCCESS;
}
