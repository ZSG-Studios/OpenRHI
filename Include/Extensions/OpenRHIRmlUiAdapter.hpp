// OpenRHI bridge for the unmodified RmlUi 6.3 public RenderInterface
#pragma once

#include "OpenRHIRmlUi.h"
#include <RmlUi/Core/RenderInterface.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

namespace openrhi {

// Host callbacks create resources or queue uploads; release callbacks defer GPU destruction through completion
// Geometry records remain valid through frame collection; consume source bytes with memcpy during CompileGeometry
// The vertex pointer is a layout-checked byte view of Rml::Vertex storage
struct RmlUiHostCallbacks {
    void* user = nullptr;
    RmlUiGeometry* (*compileGeometry)(void*, const RmlUiVertex*, uint32_t, const uint32_t*, uint32_t) = nullptr;
    void (*releaseGeometry)(void*, RmlUiGeometry*) = nullptr;
    Descriptor* (*loadTexture)(void*, const char*, uint32_t*, uint32_t*) = nullptr;
    Descriptor* (*generateTexture)(void*, const uint8_t*, uint32_t, uint32_t) = nullptr;
    void (*releaseTexture)(void*, Descriptor*) = nullptr;
};

class RmlUiRenderInterface final : public Rml::RenderInterface {
public:
    RmlUiRenderInterface(const RmlUiHostCallbacks& host, Descriptor& whiteTexture, uint32_t maxDrawNum)
        : m_Host(host)
        , m_WhiteTexture(whiteTexture)
        , m_MaxDrawNum(maxDrawNum) {
        m_Draws.reserve(maxDrawNum);
        SetTransform(nullptr);
    }

    void BeginFrame(Dim2_t displaySize) {
        m_Draws.clear();
        m_DisplaySize = displaySize;
        m_Status = (displaySize.w && displaySize.h && displaySize.w <= INT16_MAX && displaySize.h <= INT16_MAX) ? Result::SUCCESS : Result::INVALID_ARGUMENT;
        if (m_UnsupportedResources)
            m_Status = Result::UNSUPPORTED;
        m_ScissorEnabled = false;
        m_Scissor = {0, 0, displaySize.w, displaySize.h};
        SetTransform(nullptr);
    }

    Result GetStatus() const { return m_Status; }
    const DrawRmlUiDesc* GetDraws() const { return m_Draws.data(); }
    uint32_t GetDrawNum() const { return m_Status == Result::SUCCESS ? uint32_t(m_Draws.size()) : 0; }

    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override {
        static_assert(sizeof(Rml::Vertex) == sizeof(RmlUiVertex), "RmlUi 6.3 vertex ABI");
        static_assert(offsetof(Rml::Vertex, position) == offsetof(RmlUiVertex, position), "RmlUi position ABI");
        static_assert(offsetof(Rml::Vertex, colour) == offsetof(RmlUiVertex, color), "RmlUi color ABI");
        static_assert(offsetof(Rml::Vertex, tex_coord) == offsetof(RmlUiVertex, uv), "RmlUi UV ABI");
        static_assert(sizeof(int) == sizeof(uint32_t), "RmlUi index ABI");

        if (!m_Host.compileGeometry || vertices.empty() || indices.empty() || vertices.size() > UINT32_MAX || indices.size() > UINT32_MAX || indices.size() % 3) {
            m_Status = Result::INVALID_ARGUMENT;

            return 0;
        }

        for (int index : indices) {
            if (index < 0 || size_t(index) >= vertices.size()) {
                m_Status = Result::INVALID_ARGUMENT;

                return 0;
            }
        }

        RmlUiGeometry* geometry = m_Host.compileGeometry(m_Host.user, reinterpret_cast<const RmlUiVertex*>(vertices.data()), uint32_t(vertices.size()), reinterpret_cast<const uint32_t*>(indices.data()), uint32_t(indices.size()));

        if (!geometry)
            m_Status = Result::OUT_OF_MEMORY;

        return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry);
    }

    void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override {
        if (m_Status != Result::SUCCESS)
            return;

        if (!handle || m_Draws.size() == m_MaxDrawNum) {
            m_Status = handle ? Result::OUT_OF_MEMORY : Result::INVALID_ARGUMENT;

            return;
        }

        DrawRmlUiDesc draw = {};
        draw.geometry = *reinterpret_cast<const RmlUiGeometry*>(handle);
        draw.texture = texture ? reinterpret_cast<Descriptor*>(texture) : &m_WhiteTexture;
        memcpy(draw.transform, m_Transform, sizeof(m_Transform));
        draw.translation[0] = translation.x;
        draw.translation[1] = translation.y;
        draw.scissor = m_ScissorEnabled ? m_Scissor : Rect{0, 0, m_DisplaySize.w, m_DisplaySize.h};
        m_Draws.push_back(draw);
    }

    void ReleaseGeometry(Rml::CompiledGeometryHandle handle) override {
        if (handle && m_Host.releaseGeometry)
            m_Host.releaseGeometry(m_Host.user, reinterpret_cast<RmlUiGeometry*>(handle));
    }

    Rml::TextureHandle LoadTexture(Rml::Vector2i& dimensions, const Rml::String& source) override {
        uint32_t width = 0, height = 0;
        Descriptor* texture = m_Host.loadTexture ? m_Host.loadTexture(m_Host.user, source.c_str(), &width, &height) : nullptr;

        if (!texture || !width || !height || width > INT32_MAX || height > INT32_MAX) {
            if (texture && m_Host.releaseTexture)
                m_Host.releaseTexture(m_Host.user, texture);

            m_Status = Result::FAILURE;

            return 0;
        }

        dimensions = {int(width), int(height)};

        return reinterpret_cast<Rml::TextureHandle>(texture);
    }

    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions) override {
        if (!m_Host.generateTexture || dimensions.x <= 0 || dimensions.y <= 0 || uint64_t(dimensions.x) * uint64_t(dimensions.y) > (source.size() / 4)) {
            m_Status = Result::INVALID_ARGUMENT;

            return 0;
        }

        Descriptor* texture = m_Host.generateTexture(m_Host.user, source.data(), uint32_t(dimensions.x), uint32_t(dimensions.y));

        if (!texture)
            m_Status = Result::FAILURE;

        return reinterpret_cast<Rml::TextureHandle>(texture);
    }

    void ReleaseTexture(Rml::TextureHandle texture) override {
        if (texture && m_Host.releaseTexture)
            m_Host.releaseTexture(m_Host.user, reinterpret_cast<Descriptor*>(texture));
    }

    void EnableScissorRegion(bool enable) override { m_ScissorEnabled = enable; }
    void SetScissorRegion(Rml::Rectanglei region) override {
        const int x = std::clamp(region.Left(), 0, int(m_DisplaySize.w));
        const int y = std::clamp(region.Top(), 0, int(m_DisplaySize.h));
        const int right = std::clamp(region.Right(), x, int(m_DisplaySize.w));
        const int bottom = std::clamp(region.Bottom(), y, int(m_DisplaySize.h));
        m_Scissor = {int16_t(x), int16_t(y), Dim_t(right - x), Dim_t(bottom - y)};
    }
    void SetTransform(const Rml::Matrix4f* transform) override {
        for (uint32_t column = 0; column < 4; column++)
            for (uint32_t row = 0; row < 4; row++)
                m_Transform[column * 4 + row] = transform ? transform->GetColumn(int(column))[int(row)] : float(column == row);
    }

    void EnableClipMask(bool enable) override { if (enable) Unsupported(); }
    void RenderToClipMask(Rml::ClipMaskOperation, Rml::CompiledGeometryHandle, Rml::Vector2f) override { Unsupported(); }
    Rml::LayerHandle PushLayer() override { Unsupported(); return 0; }
    void CompositeLayers(Rml::LayerHandle, Rml::LayerHandle, Rml::BlendMode, Rml::Span<const Rml::CompiledFilterHandle>) override { Unsupported(); }
    void PopLayer() override { Unsupported(); }
    Rml::TextureHandle SaveLayerAsTexture() override { Unsupported(); return 0; }
    Rml::CompiledFilterHandle SaveLayerAsMaskImage() override { Unsupported(); return 0; }
    Rml::CompiledFilterHandle CompileFilter(const Rml::String&, const Rml::Dictionary&) override { m_UnsupportedResources = true; Unsupported(); return 0; }
    void ReleaseFilter(Rml::CompiledFilterHandle) override {}
    Rml::CompiledShaderHandle CompileShader(const Rml::String&, const Rml::Dictionary&) override { m_UnsupportedResources = true; Unsupported(); return 0; }
    void RenderShader(Rml::CompiledShaderHandle, Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override { Unsupported(); }
    void ReleaseShader(Rml::CompiledShaderHandle) override {}

private:
    void Unsupported() { m_Status = Result::UNSUPPORTED; }
    RmlUiHostCallbacks m_Host;
    Descriptor& m_WhiteTexture;
    std::vector<DrawRmlUiDesc> m_Draws;
    Dim2_t m_DisplaySize = {};
    Rect m_Scissor = {};
    float m_Transform[16] = {};
    Result m_Status = Result::SUCCESS;
    bool m_ScissorEnabled = false;
    uint32_t m_MaxDrawNum;
    bool m_UnsupportedResources = false;
};

} // namespace openrhi
