#include "SharedExternal.h"

#include <cstdio>

// Use the exported format properties with the production inline layout and dimension helpers
#if !OPENRHI_TEST_STATIC_LIBRARY
const openrhi::FormatProps& openrhi::GetFormatProps(Format format) {
    return *openrhiGetFormatProps(format);
}
#endif

using namespace openrhi;

int main() {
    const PlaneBits planes[] = {PlaneBits::ALL, PlaneBits::DEPTH, PlaneBits::STENCIL, PlaneBits::NONE};
    const Layout layouts[] = {Layout::DEPTH_STENCIL_ATTACHMENT, Layout::DEPTH_ATTACHMENT_STENCIL_READONLY,
        Layout::DEPTH_READONLY_STENCIL_ATTACHMENT, Layout::DEPTH_STENCIL_READONLY};
    TextureViewDesc view = {};
    view.type = TextureView::DEPTH_STENCIL_ATTACHMENT;
    view.format = Format::D32_SFLOAT_S8_UINT;

    for (uint32_t i = 0; i < 4; i++) {
        view.planes = planes[i];
        view.layout = Layout::UNDEFINED;

        if (GetTextureViewLayout(view) != layouts[i])
            return 1;

        view.layout = Layout::GENERAL;

        if (GetTextureViewLayout(view) != Layout::GENERAL)
            return 2;
    }

    if (!IsDepthReadOnly(Layout::DEPTH_READONLY_STENCIL_ATTACHMENT) || IsStencilReadOnly(Layout::DEPTH_READONLY_STENCIL_ATTACHMENT)
        || IsDepthReadOnly(Layout::DEPTH_ATTACHMENT_STENCIL_READONLY) || !IsStencilReadOnly(Layout::DEPTH_ATTACHMENT_STENCIL_READONLY)
        || !IsDepthReadOnly(Layout::DEPTH_STENCIL_READONLY) || !IsStencilReadOnly(Layout::DEPTH_STENCIL_READONLY))
        return 3;

    TextureDesc texture = {};
    texture.format = Format::ASTC_5X4_UNORM;
    texture.width = texture.height = 8;
    texture.depth = 1;

    if (GetDimension(GraphicsAPI::METAL, texture, 0, 0) != 10 || GetDimension(GraphicsAPI::METAL, texture, 1, 0) != 8
        || GetDimension(GraphicsAPI::METAL, texture, 1, 2) != 4 || GetDimension(GraphicsAPI::VK, texture, 1, 2) != 2)
        return 4;

    texture.format = Format::ASTC_6X5_UNORM;

    if (GetDimension(GraphicsAPI::METAL, texture, 0, 0) != 12 || GetDimension(GraphicsAPI::METAL, texture, 1, 0) != 10)
        return 5;

    std::puts("Texture-view layout defaults, explicit overrides, plane access and rectangular ASTC dimensions: PASS");

    return 0;
}
