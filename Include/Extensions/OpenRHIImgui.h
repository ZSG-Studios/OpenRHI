// © 2025 NVIDIA Corporation

// Goal: ImGui rendering

#pragma once

#define OPENRHI_IMGUI_H 1

/*
Requirements:
- ImGui 1.92.9+ with "ImGuiBackendFlags_RendererHasTextures" flag ("IMGUI_DISABLE_OBSOLETE_FUNCTIONS" is recommended),
- unmodified "ImDrawVert" (20 bytes) and "ImDrawIdx" (2 bytes)
- expected "ImTextureData" is 96 bytes
- "ImTextureID_Invalid" = 0

Expected usage:
- the goal of this extension is to support latest ImGui only
- designed only for rendering
- "ImguiRenderData" must be used before "EndStreamerFrame" and before the associated "Streamer" host-data ring buffer wraps
- "StreamerDesc::hostDataCapacity" must accommodate all simultaneously live host-data allocations and alignment padding
- all data referenced by "CopyImguiDataDesc" must remain valid and immutable until "CmdCopyImguiData" returns
- "drawList->AddCallback" functionality is not supported! But there is a special callback, allowing to override "hdrScale":
     drawList->AddCallback(OPENRHI_IMGUI_OVERRIDE_HDR_SCALE(1000.0f)); // to override "DrawImguiDesc::hdrScale"
     drawList->AddCallback(OPENRHI_IMGUI_OVERRIDE_HDR_SCALE(0.0f));    // to revert back to "DrawImguiDesc::hdrScale"
- "ImGui::Image*" functions are supported. "ImTextureID" must be a "SHADER_RESOURCE" descriptor:
     ImGui::Image((ImTextureID)descriptor, ...)

Single-threaded usage:
- configure "StreamerDesc::hostDataCapacity", call "CmdCopyImguiData" and pass the returned "ImguiRenderData" to "CmdDrawImgui"
- record all draws before "EndStreamerFrame" and before the associated "Streamer" host-data ring buffer wraps

Multi-threaded usage:
- one "openrhi::Imgui" instance may be used to render several "ImGui" contexts from multiple threads; calls are internally serialized
- the copy/draw handoff stores no instance-shared transient state: each copy returns independent "ImguiRenderData"
- each thread must use its own command buffer and returned "ImguiRenderData"; a shared "Streamer" must follow its multi-threaded usage contract
- different "Imgui" instances may share "ImTextureData"; access is internally synchronized, but referenced input must remain immutable until every concurrent copy returns
*/

NonOpenRhiForwardStruct(ImDrawList);
NonOpenRhiForwardStruct(ImTextureData);

OpenRhiNamespaceBegin

OpenRhiForwardStruct(Imgui);
OpenRhiForwardStruct(Streamer);

OpenRhiStruct(ImguiDesc) {
    OpenRhiOptional uint32_t descriptorPoolSize;    // upper bound of textures used by Imgui for drawing across all threads: {number of queued frames} * {number of "CmdDrawImgui" calls} * (1 + {"drawList->AddImage*" calls})
};

OpenRhiStruct(CopyImguiDataDesc) {
    const ImDrawList* const* drawLists;         // ImDrawData::CmdLists.Data
    uint32_t drawListNum;                       // ImDrawData::CmdLists.Size
    ImTextureData* const* textures;             // ImDrawData::Textures->Data (same as "ImGui::GetPlatformIO().Textures.Data")
    uint32_t textureNum;                        // ImDrawData::Textures->Size (same as "ImGui::GetPlatformIO().Textures.Size")
};

OpenRhiStruct(DrawImguiDesc) {
    OpenRhi(Dim2_t) displaySize;                    // ImDrawData::DisplaySize
    float hdrScale;                             // SDR intensity in HDR mode (1 by default)
    OpenRhi(Format) attachmentFormat;               // destination attachment (render target) format
    bool linearColor;                           // apply de-gamma to vertex colors (needed for sRGB attachments and HDR)
};

// Read-only input for "CmdDrawImgui"
OpenRhiStruct(ImguiRenderData) {
    OpenRhiPtr(Imgui) imgui;                        // producing instance
    const void* drawCommands;                   // in transient Streamer host data
    OpenRhi(BufferOffset) vertices;                 // in transient Streamer buffer
    uint64_t indexBufferOffset;                 // in "vertices.buffer"
    uint32_t drawCmdNum;
};

// Threadsafe: yes, if command buffers are externally synchronized and Streamer thread safety is enabled (or provided externally).
// Access to shared "ImTextureData" is internally synchronized.
OpenRhiStruct(ImguiInterface) {
    OpenRhi(Result) (OPENRHI_CALL *CreateImgui)         (OpenRhiRef(Device) device, const OpenRhiRef(ImguiDesc) imguiDesc, OpenRhiOut OpenRhiRef(Imgui*) imgui);
    void        (OPENRHI_CALL *DestroyImgui)        (OpenRhiPtr(Imgui) imgui);

    // Command buffer
    // {
        // Copy
        void    (OPENRHI_CALL *CmdCopyImguiData)    (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(Streamer) streamer, OpenRhiRef(Imgui) imgui, const OpenRhiRef(CopyImguiDataDesc) copyImguiDataDesc, OpenRhiOut OpenRhiRef(ImguiRenderData) imguiRenderData);

        // Draw (changes descriptor pool, pipeline layout and pipeline, barriers are externally controlled)
        void    (OPENRHI_CALL *CmdDrawImgui)        (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(ImguiRenderData) imguiRenderData, const OpenRhiRef(DrawImguiDesc) drawImguiDesc);
    // }
};

OpenRhiNamespaceEnd

#define OPENRHI_IMGUI_OVERRIDE_HDR_SCALE(hdrScale) (ImDrawCallback)1, _OpenRhiCastFloatToVoidPtr(hdrScale)

inline void* _OpenRhiCastFloatToVoidPtr(float f) {
    // A strange cast is there to get a fast path in Imgui
    return *(void**)&f;
}
