// Exercise the real pinned RmlUi public virtual interface and OpenRHI bridge
#include "OpenRHI.h"

#include <cstdio>
#include <RmlUi/Core/Variant.h>
#include <stdexcept>
#include "Extensions/OpenRHIRmlUiAdapter.hpp"

using namespace openrhi;

static void Require(bool value) {
    if (!value)
        throw std::runtime_error("RmlUi bridge contract");
}

struct Host {
    RmlUiGeometry geometry = {};
    uint32_t geometryReleases = 0;
    uint32_t textureReleases = 0;
};

static RmlUiGeometry* Compile(void* user, const RmlUiVertex* vertices, uint32_t vertexNum, const uint32_t* indices, uint32_t indexNum) {
    Host& host = *(Host*)user;
    Require(vertexNum == 3 && indexNum == 3 && indices[2] == 2);
    RmlUiVertex copied[3];
    memcpy(copied, vertices, sizeof(copied));
    Require(copied[1].position[0] == 8 && copied[0].color[3] == 255);
    host.geometry.vertexNum = vertexNum;
    host.geometry.indexNum = indexNum;

    return &host.geometry;
}

int main() {
    try {
        Host host;
        auto* white = reinterpret_cast<Descriptor*>(uintptr_t(16));
        RmlUiHostCallbacks callbacks = {};
        callbacks.user = &host;
        callbacks.compileGeometry = Compile;
        callbacks.releaseGeometry = [](void* user, RmlUiGeometry*) { ((Host*)user)->geometryReleases++; };
        callbacks.generateTexture = [](void*, const uint8_t* source, uint32_t width, uint32_t height) -> Descriptor* {
            Require(width == 1 && height == 1 && source[3] == 255);

            return reinterpret_cast<Descriptor*>(uintptr_t(32));
        };
        callbacks.loadTexture = [](void*, const char*, uint32_t* width, uint32_t* height) -> Descriptor* {
            *width = *height = 1;

            return reinterpret_cast<Descriptor*>(uintptr_t(32));
        };
        callbacks.releaseTexture = [](void* user, Descriptor*) { ((Host*)user)->textureReleases++; };
        RmlUiRenderInterface bridge(callbacks, *white, 2);
        Rml::RenderInterface& api = bridge;
        bridge.BeginFrame({32, 32});
        Rml::Vertex vertices[3] = {};
        vertices[0].colour = {255, 255, 255, 255};
        vertices[1].position.x = 8;
        int indices[] = {0, 1, 2};
        const auto geometry = api.CompileGeometry({vertices, 3}, {indices, 3});
        Require(geometry != 0);
        Rml::Matrix4f transform = Rml::Matrix4f::Identity();
        transform.data()[12] = 7;
        api.SetTransform(&transform);
        api.SetScissorRegion(Rml::Rectanglei::FromPositionSize({-4, 3}, {14, 8}));
        api.EnableScissorRegion(true);
        api.RenderGeometry(geometry, {2, 4}, 0);
        Require(bridge.GetDrawNum() == 1 && bridge.GetDraws()[0].texture == white);
        const auto& draw = bridge.GetDraws()[0];
        Require(draw.transform[12] == 7 && draw.translation[0] == 2 && draw.scissor.x == 0 && draw.scissor.width == 10);
        uint8_t pixel[] = {255, 255, 255, 255};
        auto texture = api.GenerateTexture({pixel, 4}, {1, 1});
        Require(texture != 0);
        api.SetTransform(nullptr);
        api.RenderGeometry(geometry, {0, 0}, texture);
        Require(bridge.GetDrawNum() == 2 && bridge.GetDraws()[1].transform[12] == 0);
        api.RenderGeometry(geometry, {0, 0}, texture);
        Require(bridge.GetStatus() == Result::OUT_OF_MEMORY && bridge.GetDrawNum() == 0);
        bridge.BeginFrame({32, 32});
        api.EnableClipMask(true);
        Require(bridge.GetStatus() == Result::UNSUPPORTED && bridge.GetDrawNum() == 0);
        bridge.BeginFrame({32, 32});
        Require(api.PushLayer() == 0 && bridge.GetStatus() == Result::UNSUPPORTED);
        bridge.BeginFrame({32, 32});
        indices[0] = -1;
        Require(api.CompileGeometry({vertices, 3}, {indices, 3}) == 0 && bridge.GetStatus() == Result::INVALID_ARGUMENT);
        api.ReleaseGeometry(geometry);
        api.ReleaseTexture(texture);
        Require(host.geometryReleases == 1 && host.textureReleases == 1);
        api.CompileFilter("blur", {});
        bridge.BeginFrame({32, 32});
        Require(bridge.GetStatus() == Result::UNSUPPORTED && bridge.GetDrawNum() == 0);
        std::puts("RmlUi 6.3 bridge: geometry, textures, transforms, scissor, bounded collection, unsupported-feature rejection PASS");

        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());

        return 1;
    }
}
