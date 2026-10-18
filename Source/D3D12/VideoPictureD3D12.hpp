// © 2026 NVIDIA Corporation

OPENRHI_INLINE Result VideoPictureD3D12::Create(const VideoPictureDesc& videoPictureDesc) {
    m_Texture = (TextureD3D12*)videoPictureDesc.texture;

    const TextureDesc& textureDesc = m_Texture->GetDesc();
    m_Subresource = GetSubresourceIndex(videoPictureDesc.layer, textureDesc.layerNum, 0, textureDesc.mipNum, PlaneBits::COLOR);

    return Result::SUCCESS;
}
