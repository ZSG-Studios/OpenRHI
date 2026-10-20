// OpenRHI RmlUi rendering extension
#pragma once

#define OPENRHI_RMLUI_H 1

OpenRhiNamespaceBegin

OpenRhiForwardStruct(RmlUi);

// Matches RmlUi 6.3's public Vertex layout; colors and textures use premultiplied alpha
OpenRhiStruct(RmlUiVertex) {
    float position[2];
    uint8_t color[4];
    float uv[2];
};

OpenRhiStruct(RmlUiCapabilities) {
    bool nativeRendering;
    bool geometry;
    bool textures;
    bool scissor;
    bool transforms;
    bool clipMasks;
    bool layers;
    bool filters;
    bool customShaders;
};

OpenRhiStruct(RmlUiDesc) {
    OpenRhi(Format) attachmentFormat; // single-sample color target
    uint32_t maxDrawNum; // descriptor budget for all recordings before ResetRmlUi; nonzero
    bool linearColor; // convert premultiplied sRGB UI output to linear for sRGB/HDR attachments; texture views use UNORM
    float hdrScale; // 0 selects 1; SDR intensity on HDR targets
};

// Caller-owned GPU buffers; indices are unsigned 32-bit, local to vertices
OpenRhiStruct(RmlUiGeometry) {
    OpenRhi(BufferOffset) vertices;
    OpenRhi(BufferOffset) indices;
    uint32_t vertexNum;
    uint32_t indexNum;
};

OpenRhiStruct(DrawRmlUiDesc) {
    OpenRhi(RmlUiGeometry) geometry;
    OpenRhiPtr(Descriptor) texture; // sampled RGBA texture; use caller-owned opaque white for untextured geometry
    float transform[16]; // column-major; identity when no RmlUi transform is active
    float translation[2]; // applied before transform
    OpenRhi(Rect) scissor; // nonnegative window coordinates, independent of transform
};

// Externally synchronize each instance; caller owns resources, barriers, rendering scope and submission
// Keep every referenced resource alive until GPU completion; Reset/Destroy require all prior uses to complete
// CmdDrawRmlUi records in order inside rendering and changes descriptor pool/layout, pipeline, viewport, scissor and geometry bindings
OpenRhiStruct(RmlUiInterface) {
    OpenRhi(RmlUiCapabilities) (OPENRHI_CALL *GetRmlUiCapabilities) (const OpenRhiRef(Device) device);
    OpenRhi(Result) (OPENRHI_CALL *CreateRmlUi) (OpenRhiRef(Device) device, const OpenRhiRef(RmlUiDesc) desc, OpenRhiOut OpenRhiRef(RmlUi*) rmlUi);
    void (OPENRHI_CALL *DestroyRmlUi) (OpenRhiPtr(RmlUi) rmlUi);
    void (OPENRHI_CALL *ResetRmlUi) (OpenRhiRef(RmlUi) rmlUi);
    OpenRhi(Result) (OPENRHI_CALL *CmdDrawRmlUi) (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(RmlUi) rmlUi, OpenRhi(Dim2_t) displaySize, const OpenRhiPtr(DrawRmlUiDesc) draws, uint32_t drawNum);
};

OpenRhiNamespaceEnd
