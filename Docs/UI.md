# UI renderers

OpenRHI owns native rendering through `ImguiInterface` and `RmlUiInterface`. MIRA owns ImGui/RmlUi contexts, application documents, SDL input/window integration, asset decoding, resource upload scheduling, and application UI. Enable both renderers with `-DOPENRHI_ENABLE_IMGUI_EXTENSION=ON -DOPENRHI_ENABLE_RMLUI_EXTENSION=ON` (the maintained default).

## Pins and host integration contract

- ImGui: **1.92.9**, commit **01380c579715e62fb9a8d6ec0502c4ea83bfde6e**, default 20-byte `ImDrawVert`, 16-bit `ImDrawIdx`, 96-byte `ImTextureData`, `ImTextureID_Invalid=0`, `ImGuiBackendFlags_RendererHasTextures`. Include `OpenRHI.h`, `Extensions/OpenRHIImgui.h`, and `Extensions/OpenRHIStreamer.h`; query `ImguiInterface` and `StreamerInterface` from the existing device.
- RmlUi: **6.3**, tag commit **ba95ffe8bfb6370efb2cdcca927eaad4710c5413** from [official RmlUi](https://github.com/mikke89/RmlUi/tree/ba95ffe8bfb6370efb2cdcca927eaad4710c5413), MIT. The adapter uses the public `Rml::RenderInterface` in `RmlUi/Core/RenderInterface.h`: `CompileGeometry(Span<const Vertex>, Span<const int>)`, `RenderGeometry(handle, translation, texture)`, texture callbacks, scissor callbacks, and column-major `SetTransform`. Vendor source remains unmodified.
- Native RmlUi facade: include `OpenRHI.h` and `Extensions/OpenRHIRmlUi.h`, query `RmlUiInterface`, create one renderer per attachment format and in-flight descriptor budget, and record `CmdDrawRmlUi` inside caller-owned rendering. Geometry/index buffers, texture descriptors, transitions, submissions, and fences are caller owned.
- Optional C++ host bridge: include `Extensions/OpenRHIRmlUiAdapter.hpp` after making the pinned RmlUi headers available. `openrhi::RmlUiRenderInterface` implements the RmlUi 6.3 callbacks. Supply callbacks that create/release caller-owned GPU geometry and texture descriptors, plus a white premultiplied RGBA texture descriptor for untextured geometry. Link OpenRHI and the same RmlUi core used by the application's context. OpenRHI's native library has no RmlUi runtime dependency.

The bridge collects a bounded list of draw descriptors during `Rml::Context::Render()`. Call `BeginFrame`, render the context, then check `GetStatus`. Flush host-scheduled geometry/texture uploads and transitions outside rendering; begin the target rendering scope and pass `GetDraws()`/`GetDrawNum()` to `CmdDrawRmlUi`. The host retains resources through GPU completion, including geometry/textures released by RmlUi during frame collection. Reset the renderer's descriptor budget with `ResetRmlUi` only after every prior use of that renderer has completed; separate per-frame renderer instances permit multiple frames in flight.

Capabilities explicitly describe geometry, premultiplied textures/text, window-coordinate scissor, and column-major transforms. Advanced clip masks, layers, filters, and custom shaders are separate capability gates. The bridge returns `UNSUPPORTED` and exposes zero draws when a document requests an unavailable feature. Treat that result as a frame failure and choose documents matching the capability report.

Unsupported filter/shader compilation is latched across frames so an earlier document update cannot silently lose the error. Unload documents and recreate the bridge when replacing a document that required unavailable rendering features.

## Recording and linking

The native extension is C/C++ compatible. The optional bridge is header-only C++17 and uses the application's pinned RmlUi core. The default RmlUi vertex ABI is 20 bytes (`position` at 0, premultiplied RGBA at 8, UV at 12); indices are unsigned 32-bit. Geometry callbacks consume the layout-checked vertex byte view with `memcpy` and validate/rebase indices before exposing buffers. Texture callbacks provide premultiplied RGBA8 data and sampled **UNORM** views. `RmlUiDesc::linearColor` converts combined premultiplied sRGB UI output into linear color for sRGB/HDR attachments; `hdrScale=0` selects intensity 1.

```cpp
#include "OpenRHI.h"
#include "Extensions/OpenRHIRmlUi.h"
#include "Extensions/OpenRHIRmlUiAdapter.hpp"

openrhi::RmlUiInterface ui{};
openrhi::openrhiGetInterface(device, OPENRHI_INTERFACE(openrhi::RmlUiInterface), &ui);
openrhi::RmlUiDesc desc{};
desc.attachmentFormat = openrhi::Format::RGBA8_SRGB;
desc.maxDrawNum = 1024;
desc.linearColor = true;
openrhi::RmlUi* renderer = nullptr;
// Check every Result before using the returned object
ui.CreateRmlUi(device, desc, renderer);
// Once prior GPU use is complete:
ui.ResetRmlUi(*renderer);
bridge.BeginFrame(displaySize);
context.Render();
if (bridge.GetStatus() == openrhi::Result::SUCCESS) {
    // Flush caller uploads/barriers and begin caller-owned rendering here
    ui.CmdDrawRmlUi(commandBuffer, *renderer, displaySize, bridge.GetDraws(), bridge.GetDrawNum());
}
```

`bridge` is an `openrhi::RmlUiRenderInterface` constructed with `RmlUiHostCallbacks`, the caller's white texture descriptor, and collection capacity; `context` is the caller's RmlUi context. Bind it through `Rml::SetRenderInterface` before RmlUi initialization. The draw target is one single-sample color attachment matching the format used at creation. Provision the native descriptor budget for every draw recorded before reset. Exhaustion returns `OUT_OF_MEMORY` before recording that batch; it preserves earlier recorded draws. Empty batches succeed.

After either UI renderer, restore the host descriptor pool, pipeline layout/pipeline, descriptor bindings, vertex/index bindings, viewport and scissor needed for subsequent draws. For ImGui streaming, set both `constantBufferMemoryLocation` and `dynamicBufferMemoryLocation` to `HOST_UPLOAD` (or a supported `DEVICE_UPLOAD` choice), even when constant capacity is zero. Give the dynamic buffer `VERTEX | INDEX` usage, sufficient host-data capacity, and the correct queued-frame count. Copy ImGui data before rendering, draw it inside rendering, and call `EndStreamerFrame` after all recording that uses its transient data.

Link the `OpenRHI` CMake target when composing the source checkout. With the Windows SDK, add `_OpenRHI_SDK/Include`, link `Lib/<configuration>/OpenRHI.lib`, and deploy the matching DLL, Agility SDK, and licensed optional runtimes as described in [MIRA integration](mira-integration.md). Link the application's ImGui core and `RmlUi::Core` separately; SDL and FreeType belong to that UI host. The SDK includes both public extension headers, the RmlUi bridge, this guide, and embedded native UI shader binaries in OpenRHI. Preserve each UI library's original license when deploying the application-owned cores.

## Build and qualification

```powershell
cmake -S C:/OpenRHI -B C:/OpenRHI/_Build -DOPENRHI_ENABLE_IMGUI_EXTENSION=ON -DOPENRHI_ENABLE_RMLUI_EXTENSION=ON -DOPENRHI_BUILD_TESTS=ON -DOPENRHI_ENABLE_NONE_SUPPORT=ON
cmake --build C:/OpenRHI/_Build --config Release --parallel 8
cmake --build C:/OpenRHI/_Build --config Debug --parallel 8
ctest --test-dir C:/OpenRHI/_Build -C Release --output-on-failure
ctest --test-dir C:/OpenRHI/_Build -C Debug --output-on-failure
& C:/OpenRHI/Scripts/Windows/3-PrepareSDK.bat
```

Tests fetch unmodified pinned UI sources solely to qualify the host bridge/ABI. The RmlUi commit archive SHA-256 is `ed3162ef67d24b7d91557d710b3734433cfc2a8659c55b4d68a11de38b178487`; ImGui's `v1.92.9` tag archive SHA-256 is `bbdcee6a1d56e38ccddaa58383182b348cfb4f2fc59f47a9ed9ef31f8f966351`. RmlUi bridge tests use its core with the font engine set to `none`; application text/font qualification belongs to the host's actual font engine and document workload.

Native tests `OpenRHI.<backend>.UI` verify pixel readback for geometry, sampled textures, premultiplied blending, transforms, scissor, descriptor exhaustion/reset, sRGB conversion, rejected public inputs, and ImGui atlas upload/stream/draw. `OpenRHI.RmlUi.Adapter` exercises the actual 6.3 virtual interface, bounded collection and advanced-feature rejection. `OpenRHI.RmlUi.Availability` exercises NONE/Validation creation and disabled-extension discovery. `OpenRHI.RmlUi.Shaders` checks SPIR-V and Metal-source reflection: 96-byte constants, Metal root slot 2, sampler table at byte 96 and texture table at byte 104. Native Metal table entries retain the shared 24-byte ABI.

Qualification on Windows x64 with Visual Studio 18 2026, Windows SDK 10.0.28000.0 and Agility SDK 1.619.6: Debug and Release each pass **19/19 tests**, including native D3D12 UI readback on the AMD Radeon RX 9070 XT with zero unexpected validation errors. UI-disabled NONE-only Release builds pass **10 tests**, with the native adapter-selection test skipped. SPIR-V and Metal-source compilation/reflection pass. The options metadata snapshot and UI headers are included in SDK packaging checks.

This change is qualified against source baseline `85caac6dc1639573b7f721fa51758aefc43ea69b` plus the UI/options change. Build/test logs and final SDK hashes are retained under `_Tmp/UI/`; the published Git commit identifies the final source. Vulkan/Metal runtime, Apple metallib assembly, mobile execution, advanced UI effects, and performance qualification require their respective native hosts/workloads; Windows cross-compilation establishes shader-source/reflection coverage for those backends.

Module-owned rendering-option metadata and exact FSR, Anti-Lag, frame-generation and other vendor prerequisites are maintained in [OPTIONS.md](OPTIONS.md).
