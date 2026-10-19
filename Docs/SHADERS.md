# Slang shaders

OpenRHI authors and compiles shaders with Slang **2026.20**. The reusable `CMake/OpenRHISlang.cmake` module selects the compiler for the build host, verifies its version, and downloads distributions using pinned SHA-256 hashes. `OPENRHI_SLANG_ROOT` supplies an existing distribution of that exact version.

| Runtime platform | Slang target | Pipeline input |
|---|---|---|
| Windows | `dxil`, `sm_6_6` | DXIL binary |
| Linux / Android | direct `spirv`, SPIR-V 1.6 | SPIR-V binary |
| macOS / iOS / iPadOS | `metal`, Metal 4 | native metallib |

DXIL generation uses Slang's downstream DXC **library**, pinned to Microsoft release `v1.9.2609`. `OPENRHI_SLANG_DXIL_ROOT` supplies that distribution. Apple builds assemble Slang-generated Metal with Xcode's `metal` and `metallib` tools. These are stages of the Slang compilation chain. OpenRHI's runtime library loads native binaries; compilation runs explicitly during the application build.

```cmake
include(path/to/OpenRHI/CMake/OpenRHISlang.cmake)
openrhi_compile_shader(
    NAME ParticleUpdate
    SOURCE shaders/Particles.slang
    ENTRY updateParticles
    STAGE compute
    INCLUDES "${OPENRHI_INCLUDE_DIR}"
    DEFINES PARTICLE_GROUP_SIZE=64
    HEADER particle_header
    OUTPUT particle_binary)
add_custom_target(ParticleShaders DEPENDS "${particle_header}")
add_dependencies(MyApp ParticleShaders)
target_include_directories(MyApp PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
```

Include `OpenRHI.h` before the generated header. Each generated header contains `g_<NAME>` binary data, `g_<NAME>_entry`, `g_<NAME>_threads`, and `g_<NAME>_desc`, a complete C++ `openrhi::ShaderDesc`. The entry name and threadgroup dimensions come from Slang reflection. Keep the descriptor with the matching binary. For task/mesh payloads, set `TASK_PAYLOAD_SIZE` to the declared payload byte size; it populates `ShaderDesc::taskPayloadSize`, which Metal uses to allocate pipeline payload storage. C consumers can load the binary and read the adjacent JSON reflection. Imported shader files are tracked through Slang dependency files, so changes rebuild their consumers. Diagnostics are errors, and optimized compilation removes unused resources for each entry.

```slang
#include "OpenRHI.slang"
[[vk::binding(0, 0)]] RWStructuredBuffer<uint> values : register(u0, space0);
[shader("compute")]
[numthreads(64, 1, 1)]
void updateParticles(uint3 id : SV_DispatchThreadID) {
    values[id.x] += 1;
}
```

Use descriptive global entry names (for example `updateParticles`; Metal reserves `main`), `[shader]`, `[numthreads]`, resource bindings, and shader capability checks. The example binding matches D3D12 and Vulkan descriptor-set layouts; native Metal uses the root-table ABI below. Compile newer optional features and wave-size variants explicitly, then select them using the queried device capabilities. NIS selects half precision through `shaderFeatures.nativeF16` and uses the generic 256-thread policy. Metal uses 24-row half-precision tiles and 16-row full-precision tiles to keep instrumented threadgroup storage within 32 KiB; the dispatch geometry matches the selected shader.

## Native Metal tables

`OpenRHI.slang` supplies typed texture, sampler, storage-buffer, acceleration-structure, sample-mask, and metadata helpers. ImGui, NIS, and descriptor compute tests demonstrate the ABI with ordinary Slang structures and typed `DescriptorHandle` fields.

| Buffer slot | Contents |
|---|---|
| 0 | Resource heap |
| 1 | Sampler heap |
| 2 | Root data |
| 3 | Multiview indices for graphics |
| 6 + bindingSlot | Vertex stream |

A descriptor entry occupies 24 bytes: buffer GPU address or sampler ID at byte 0, texture ID at byte 8, and metadata at byte 16. Metadata holds buffer byte length in bits 0–31, typed-buffer element offset in bits 32–39, and the typed-buffer flag in bit 63. Sampler metadata contains the float mip bias on families preceding Apple10; apply `OpenRhiGetSamplerMipBias` to implicit-LOD sampling there.

Root data contains, in order:

1. Requested emulated draw base vertex / base instance, followed by requested draw index.
2. Tightly packed application root constants.
3. Root descriptor GPU addresses, aligned to eight bytes.
4. For each descriptor set, its resource-table pointer followed by its sampler-table pointer, including only the tables present in that set.
5. The root-sampler table pointer when root samplers are present.

The complete block is padded to 16 bytes. Descriptor ranges retain declaration order; a variable-sized range occupies the final range. Match the Slang structure offsets to this layout and inspect the generated reflection when defining a new layout. Bind it with `ConstantBuffer<YourRoot> root : register(b2)` and an explicit `[[vk::binding(2, 0)]]` attribute. Use the typed entry matching the resource at each slot. Mipmapped 1D resources use height-one 2D textures: Y is zero for loads/stores and 0.5 for normalized sampling.

Metal fragment shaders apply `OpenRhiApplyPipelineSampleMask` to an `SV_Coverage` output to opt into the pipeline sample-mask contract. Function constant index 65535 is reserved for this purpose. Fragment target indices belong on output structure fields, such as `float4 color : SV_Target3`. Multiview uses the dense amplification index to access the set-bit view indices supplied at buffer 3. Slang-generated Metal supports inline ray queries with the native acceleration-structure header.

Draw-parameter consumers explicitly match the pipeline layout. Vulkan uses Slang's `SV_StartVertexLocation`, `SV_StartInstanceLocation`, and `SV_DrawIndex`. D3D12 SM 6.6 uses `ENABLE_DRAW_PARAMETERS_EMULATION` with `b0, space999` (base vertex and instance), and `ENABLE_DRAW_INDEX_EMULATION` with `b1, space999` (draw index). Metal places these requested values at the start of root data. `OpenRHI.slang` supplies the matching indirect-command filling macros for the normal and emulated draw layouts.

## Migration and verification

Rename application shader sources and includes to `.slang` / `OpenRHI.slang`, define named entries, and adopt `openrhi_compile_shader`. Rebuild all native binaries and pipeline caches. Metal consumes the resulting metallib and reflection metadata. Shader compilation helpers ship with the SDK.

The build compiles ImGui, every NIS permutation, qualification shaders, and all native Metal helper kernels through Slang. Source-boundary tests enforce the authored language and compilation pipeline. Runtime correctness tests read back descriptor compute and indirect rendering results, with native validation enabled. Device receipts record actual execution separately from cross-compilation coverage.

The compiler distribution is maintained by [shader-slang](https://github.com/shader-slang/slang/releases/tag/v2026.20). NIS retains NVIDIA's MIT license and [upstream algorithm provenance](https://github.com/NVIDIAGameWorks/NVIDIAImageScaling). Slang target-specific intrinsics in the internal MSAA helper explicitly convert sample coordinates for Apple's unsigned texture-read ABI; they remain part of the single Slang source and build chain.
