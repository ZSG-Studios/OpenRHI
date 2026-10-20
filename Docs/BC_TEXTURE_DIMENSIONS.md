# BC texture dimensions and native admission

## Confirmed SDK94 D3D12 shape mismatch

The material fixture at source `7303453b7716d6306a946edf1838afcc7132010f` created BC4/BC5/BC7 ordinary 2D resources with requested original dimensions 17x9. Public texture/view factories succeeded. On RX 9070 XT, Debug and Release SDK94 shaders both reported 20x12 for the full view and 10x6 for a view beginning at mip one. The requested chain requires 17x9 and 8x4. All eight odd-original profiles failed the exact-shape guard with zero native/OpenRHI validation errors. Heap and conventional paths observed the same dimensions. `_Tmp/BindlessMaterial/Native-Policy2/publication.json` rejects that profile and retains the passing aligned subset.

The owned source path is concrete:

1. `Source/D3D12/TextureD3D12.hpp`, `TextureD3D12::Create`, retains `FixTextureDesc(textureDesc)`.
2. `Source/Shared/SharedExternal.h`, `FixTextureDesc`, resolves optional zero counts/extents and preserves requested width/height.
3. `Source/D3D12/DeviceD3D12.hpp`, `DeviceD3D12::GetResourceDesc`, rounds native width/height to format block dimensions. Allocation and memory-query paths consume that description.
4. The device initializer already queries `D3D12_FEATURE_D3D12_OPTIONS8`, but `UnalignedBlockTexturesSupported` is not retained or exposed in SDK94's public capabilities. This fixture therefore measures the rounded allocation, rather than an eligible logical-dimension-preserving native attempt.
5. Validation creation accepts the request, so factory success does not reject or explain the logical/native mismatch.

This is an OpenRHI-owned creation/public-shape defect. It changes normalized addressing and mip footprints before consumer shaders run. FACET should retain its exact dimension guard; UV rescaling would not preserve original repeat/mip semantics. Native eligibility has not been independently queried through SDK94; the mismatch is established regardless of whether that device could support an unaligned native allocation.

## Current host boundary

With SDK94, reject BC top-level width or height that is not a multiple of four before publishing a trusted view. Preserve requested/original dimensions and verify each native view. Aligned originals can have odd lower mips: the passing 20x12 chain includes 10x6 and 5x3. Keep physical block pitch separate from logical dimensions; do not round every logical mip or reconstruct an original by shifting a resident view back up. Separate resident allocations retain their own exact allocation/view provenance.

FACET reported a CPU scan of 111 actual cooked Intel textures: all were 4096x4096 or 2048x2048. That content-specific result avoids the current boundary for those materials; image/footprint and sampler gates remain independent.

## Owned fix plan

This is proposed work requiring a separate source/SDK handshake. SDK94 and normal binaries remain unchanged during fixture work.

- [ ] Retain the native OPTIONS8 result and make shape admission discoverable through the public capability/compatibility contract. A failed query supplies no positive evidence. Set the final public field/query spelling with the paired C/C++ and backend change.
- [ ] Preserve logical dimensions in native BC descriptions when eligible; otherwise return explicit unsupported with a null texture before allocation/publication. Use the same decision for placed/committed creation and memory queries. Physical block alignment remains separate from shader extent.
- [ ] Pair discovery, Validation, backend creation, wrapped-resource provenance, upload/copy footprints, SDK metadata and consumer admission. Preserve public descriptor factories and vendor allocator interfaces.
- [ ] Audit backend parity. Vulkan create-info fills native extents from logical dimensions; Metal descriptors set requested width/height. Those source paths lack D3D12's rounding, but each native device/format still needs qualification. NONE retains its test-only role.
- [ ] Add CPU tests for aligned/unaligned/odd/rectangular and saturated one-pixel chains, query failure/unsupported decisions, effective view dimensions and REMAINING normalization.
- [ ] In a new native window, record actual OPTIONS8 eligibility and exact shader dimensions for original/partial views. Require independent decode/filter readback, repeat seams, uploaded tail mips, heap/conventional parity and zero validation errors. Preserve aligned baselines and prove unsupported requests fail before recording.
- [ ] Verify Debug/Release and supported Agility/non-Agility headers, inspect final sources, then produce a separately identified SDK and coordinate consumer rebuild/image/footprint gates before production admission changes.

Microsoft's [OPTIONS8 contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_d3d12_options8) distinguishes top-level BC alignment eligibility from logical lower-mip dimensions.
