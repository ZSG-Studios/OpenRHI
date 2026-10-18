// © 2026 NVIDIA Corporation
// Native Metal backend helpers, authored and compiled through Slang
#if defined(OPENRHI_INTERNAL_CLEAR)
struct ClearConstants { float4 color; float depth; };
[[vk::binding(3, 0)]] ConstantBuffer<ClearConstants> clear : register(b3);
struct ClearVertex { float4 position : SV_Position; uint layer : SV_RenderTargetArrayIndex; };
[shader("vertex")]
ClearVertex openrhi_clear_vs(uint i : SV_VertexID, uint layer : SV_InstanceID) {
    const float2 positions[3] = {float2(-1, -1), float2(3, -1), float2(-1, 3)};
    ClearVertex output;
    output.position = float4(positions[i], clear.depth, 1);
    output.layer = layer;

    return output;
}
struct ClearOutput_0_0 { float4 value : SV_Target0; };
[shader("fragment")]
ClearOutput_0_0 openrhi_clear_fs_0_0() { ClearOutput_0_0 output; output.value = clear.color; return output; }
struct ClearOutput_1_0 { uint4 value : SV_Target0; };
[shader("fragment")]
ClearOutput_1_0 openrhi_clear_fs_1_0() { ClearOutput_1_0 output; output.value = asuint(clear.color); return output; }
struct ClearOutput_2_0 { int4 value : SV_Target0; };
[shader("fragment")]
ClearOutput_2_0 openrhi_clear_fs_2_0() { ClearOutput_2_0 output; output.value = asint(clear.color); return output; }
struct ClearOutput_0_1 { float4 value : SV_Target1; };
[shader("fragment")]
ClearOutput_0_1 openrhi_clear_fs_0_1() { ClearOutput_0_1 output; output.value = clear.color; return output; }
struct ClearOutput_1_1 { uint4 value : SV_Target1; };
[shader("fragment")]
ClearOutput_1_1 openrhi_clear_fs_1_1() { ClearOutput_1_1 output; output.value = asuint(clear.color); return output; }
struct ClearOutput_2_1 { int4 value : SV_Target1; };
[shader("fragment")]
ClearOutput_2_1 openrhi_clear_fs_2_1() { ClearOutput_2_1 output; output.value = asint(clear.color); return output; }
struct ClearOutput_0_2 { float4 value : SV_Target2; };
[shader("fragment")]
ClearOutput_0_2 openrhi_clear_fs_0_2() { ClearOutput_0_2 output; output.value = clear.color; return output; }
struct ClearOutput_1_2 { uint4 value : SV_Target2; };
[shader("fragment")]
ClearOutput_1_2 openrhi_clear_fs_1_2() { ClearOutput_1_2 output; output.value = asuint(clear.color); return output; }
struct ClearOutput_2_2 { int4 value : SV_Target2; };
[shader("fragment")]
ClearOutput_2_2 openrhi_clear_fs_2_2() { ClearOutput_2_2 output; output.value = asint(clear.color); return output; }
struct ClearOutput_0_3 { float4 value : SV_Target3; };
[shader("fragment")]
ClearOutput_0_3 openrhi_clear_fs_0_3() { ClearOutput_0_3 output; output.value = clear.color; return output; }
struct ClearOutput_1_3 { uint4 value : SV_Target3; };
[shader("fragment")]
ClearOutput_1_3 openrhi_clear_fs_1_3() { ClearOutput_1_3 output; output.value = asuint(clear.color); return output; }
struct ClearOutput_2_3 { int4 value : SV_Target3; };
[shader("fragment")]
ClearOutput_2_3 openrhi_clear_fs_2_3() { ClearOutput_2_3 output; output.value = asint(clear.color); return output; }
struct ClearOutput_0_4 { float4 value : SV_Target4; };
[shader("fragment")]
ClearOutput_0_4 openrhi_clear_fs_0_4() { ClearOutput_0_4 output; output.value = clear.color; return output; }
struct ClearOutput_1_4 { uint4 value : SV_Target4; };
[shader("fragment")]
ClearOutput_1_4 openrhi_clear_fs_1_4() { ClearOutput_1_4 output; output.value = asuint(clear.color); return output; }
struct ClearOutput_2_4 { int4 value : SV_Target4; };
[shader("fragment")]
ClearOutput_2_4 openrhi_clear_fs_2_4() { ClearOutput_2_4 output; output.value = asint(clear.color); return output; }
struct ClearOutput_0_5 { float4 value : SV_Target5; };
[shader("fragment")]
ClearOutput_0_5 openrhi_clear_fs_0_5() { ClearOutput_0_5 output; output.value = clear.color; return output; }
struct ClearOutput_1_5 { uint4 value : SV_Target5; };
[shader("fragment")]
ClearOutput_1_5 openrhi_clear_fs_1_5() { ClearOutput_1_5 output; output.value = asuint(clear.color); return output; }
struct ClearOutput_2_5 { int4 value : SV_Target5; };
[shader("fragment")]
ClearOutput_2_5 openrhi_clear_fs_2_5() { ClearOutput_2_5 output; output.value = asint(clear.color); return output; }
struct ClearOutput_0_6 { float4 value : SV_Target6; };
[shader("fragment")]
ClearOutput_0_6 openrhi_clear_fs_0_6() { ClearOutput_0_6 output; output.value = clear.color; return output; }
struct ClearOutput_1_6 { uint4 value : SV_Target6; };
[shader("fragment")]
ClearOutput_1_6 openrhi_clear_fs_1_6() { ClearOutput_1_6 output; output.value = asuint(clear.color); return output; }
struct ClearOutput_2_6 { int4 value : SV_Target6; };
[shader("fragment")]
ClearOutput_2_6 openrhi_clear_fs_2_6() { ClearOutput_2_6 output; output.value = asint(clear.color); return output; }
struct ClearOutput_0_7 { float4 value : SV_Target7; };
[shader("fragment")]
ClearOutput_0_7 openrhi_clear_fs_0_7() { ClearOutput_0_7 output; output.value = clear.color; return output; }
struct ClearOutput_1_7 { uint4 value : SV_Target7; };
[shader("fragment")]
ClearOutput_1_7 openrhi_clear_fs_1_7() { ClearOutput_1_7 output; output.value = asuint(clear.color); return output; }
struct ClearOutput_2_7 { int4 value : SV_Target7; };
[shader("fragment")]
ClearOutput_2_7 openrhi_clear_fs_2_7() { ClearOutput_2_7 output; output.value = asint(clear.color); return output; }
#elif defined(OPENRHI_INTERNAL_BUFFER)
[[vk::binding(3, 0)]] RWStructuredBuffer<uint> destination : register(u3);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[shader("compute")][numthreads(64, 1, 1)]
void openrhi_clear_storage_buffer(uint i : SV_DispatchThreadID) { destination[i] = value.x; }
#elif defined(OPENRHI_INTERNAL_STORAGE_0_0)
[[vk::binding(0, 0)]] RWBuffer<float4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_0_0(uint3 p : SV_DispatchThreadID) { destination[p.x + offset] = asfloat(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_0_1)
[[vk::binding(0, 0)]] RWBuffer<uint4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_0_1(uint3 p : SV_DispatchThreadID) { destination[p.x + offset] = value; }
#elif defined(OPENRHI_INTERNAL_STORAGE_0_2)
[[vk::binding(0, 0)]] RWBuffer<int4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_0_2(uint3 p : SV_DispatchThreadID) { destination[p.x + offset] = asint(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_1_0)
[[vk::binding(0, 0)]] RWTexture1D<float4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_1_0(uint3 p : SV_DispatchThreadID) { destination[p.x] = asfloat(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_1_1)
[[vk::binding(0, 0)]] RWTexture1D<uint4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_1_1(uint3 p : SV_DispatchThreadID) { destination[p.x] = value; }
#elif defined(OPENRHI_INTERNAL_STORAGE_1_2)
[[vk::binding(0, 0)]] RWTexture1D<int4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_1_2(uint3 p : SV_DispatchThreadID) { destination[p.x] = asint(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_2_0)
[[vk::binding(0, 0)]] RWTexture1DArray<float4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_2_0(uint3 p : SV_DispatchThreadID) { destination[p.xy] = asfloat(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_2_1)
[[vk::binding(0, 0)]] RWTexture1DArray<uint4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_2_1(uint3 p : SV_DispatchThreadID) { destination[p.xy] = value; }
#elif defined(OPENRHI_INTERNAL_STORAGE_2_2)
[[vk::binding(0, 0)]] RWTexture1DArray<int4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_2_2(uint3 p : SV_DispatchThreadID) { destination[p.xy] = asint(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_3_0)
[[vk::binding(0, 0)]] RWTexture2D<float4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_3_0(uint3 p : SV_DispatchThreadID) { destination[p.xy] = asfloat(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_3_1)
[[vk::binding(0, 0)]] RWTexture2D<uint4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_3_1(uint3 p : SV_DispatchThreadID) { destination[p.xy] = value; }
#elif defined(OPENRHI_INTERNAL_STORAGE_3_2)
[[vk::binding(0, 0)]] RWTexture2D<int4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_3_2(uint3 p : SV_DispatchThreadID) { destination[p.xy] = asint(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_4_0)
[[vk::binding(0, 0)]] RWTexture2DArray<float4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_4_0(uint3 p : SV_DispatchThreadID) { destination[p] = asfloat(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_4_1)
[[vk::binding(0, 0)]] RWTexture2DArray<uint4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_4_1(uint3 p : SV_DispatchThreadID) { destination[p] = value; }
#elif defined(OPENRHI_INTERNAL_STORAGE_4_2)
[[vk::binding(0, 0)]] RWTexture2DArray<int4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_4_2(uint3 p : SV_DispatchThreadID) { destination[p] = asint(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_5_0)
[[vk::binding(0, 0)]] RWTexture3D<float4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_5_0(uint3 p : SV_DispatchThreadID) { destination[uint3(p.xy, p.z + offset)] = asfloat(value); }
#elif defined(OPENRHI_INTERNAL_STORAGE_5_1)
[[vk::binding(0, 0)]] RWTexture3D<uint4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_5_1(uint3 p : SV_DispatchThreadID) { destination[uint3(p.xy, p.z + offset)] = value; }
#elif defined(OPENRHI_INTERNAL_STORAGE_5_2)
[[vk::binding(0, 0)]] RWTexture3D<int4> destination : register(u0);
[[vk::binding(4, 0)]] ConstantBuffer<uint4> value : register(b4);
[[vk::binding(5, 0)]] ConstantBuffer<uint> offset : register(b5);
[shader("compute")][numthreads(8, 8, 1)]
void openrhi_clear_storage_5_2(uint3 p : SV_DispatchThreadID) { destination[uint3(p.xy, p.z + offset)] = asint(value); }
#elif defined(OPENRHI_INTERNAL_RESOLVE_VS)
[shader("vertex")]
float4 openrhi_resolve_vs(uint i : SV_VertexID) : SV_Position {
    const float2 positions[3] = {float2(-1, -1), float2(3, -1), float2(-1, 3)};

    return float4(positions[i], 0, 1);
}
#elif defined(OPENRHI_INTERNAL_RESOLVE_2d_0)
// Slang 2026.20 emits signed MSAA coordinates; Metal requires unsigned coordinates
float4 ReadSample(Texture2DMS<float4> image, uint2 coordinate, uint sample) {
    __intrinsic_asm "$0.read(uint2($1), uint($2))";
}
struct ResolveConstants { uint2 origin; uint layer; uint samples; uint op; };
[[vk::binding(3, 0)]] ConstantBuffer<ResolveConstants> constants : register(b3);
[[vk::binding(0, 0)]] Texture2DMS<float4> image : register(t0);
[shader("fragment")]
float4 openrhi_resolve_2d_0(float4 p : SV_Position) : SV_Target0 {
    uint2 q = uint2(p.xy) + constants.origin;
    float4 value = ReadSample(image, q, 0);

    for (uint i = 1; i < constants.samples; i++) {
        float4 sample = ReadSample(image, q, i);
        value = constants.op == 0 ? value + sample : (constants.op == 1 ? min(value, sample) : max(value, sample));
    }

    return constants.op == 0 ? value / float(constants.samples) : value;
}
#elif defined(OPENRHI_INTERNAL_RESOLVE_2d_1)
// Slang 2026.20 emits signed MSAA coordinates; Metal requires unsigned coordinates
uint4 ReadSample(Texture2DMS<uint4> image, uint2 coordinate, uint sample) {
    __intrinsic_asm "$0.read(uint2($1), uint($2))";
}
struct ResolveConstants { uint2 origin; uint layer; uint samples; uint op; };
[[vk::binding(3, 0)]] ConstantBuffer<ResolveConstants> constants : register(b3);
[[vk::binding(0, 0)]] Texture2DMS<uint4> image : register(t0);
[shader("fragment")]
uint4 openrhi_resolve_2d_1(float4 p : SV_Position) : SV_Target0 {
    uint2 q = uint2(p.xy) + constants.origin;
    uint4 value = ReadSample(image, q, 0);

    for (uint i = 1; i < constants.samples; i++) {
        uint4 sample = ReadSample(image, q, i);
        value = constants.op == 0 ? value + sample : (constants.op == 1 ? min(value, sample) : max(value, sample));
    }

    return constants.op == 0 ? value / uint(constants.samples) : value;
}
#elif defined(OPENRHI_INTERNAL_RESOLVE_2d_2)
// Slang 2026.20 emits signed MSAA coordinates; Metal requires unsigned coordinates
int4 ReadSample(Texture2DMS<int4> image, uint2 coordinate, uint sample) {
    __intrinsic_asm "$0.read(uint2($1), uint($2))";
}
struct ResolveConstants { uint2 origin; uint layer; uint samples; uint op; };
[[vk::binding(3, 0)]] ConstantBuffer<ResolveConstants> constants : register(b3);
[[vk::binding(0, 0)]] Texture2DMS<int4> image : register(t0);
[shader("fragment")]
int4 openrhi_resolve_2d_2(float4 p : SV_Position) : SV_Target0 {
    uint2 q = uint2(p.xy) + constants.origin;
    int4 value = ReadSample(image, q, 0);

    for (uint i = 1; i < constants.samples; i++) {
        int4 sample = ReadSample(image, q, i);
        value = constants.op == 0 ? value + sample : (constants.op == 1 ? min(value, sample) : max(value, sample));
    }

    return constants.op == 0 ? value / int(constants.samples) : value;
}
#elif defined(OPENRHI_INTERNAL_RESOLVE_array_0)
// Slang 2026.20 emits signed MSAA coordinates; Metal requires unsigned coordinates
float4 ReadSample(Texture2DMSArray<float4> image, uint2 coordinate, uint layer, uint sample) {
    __intrinsic_asm "$0.read(uint2($1), uint($2), uint($3))";
}
struct ResolveConstants { uint2 origin; uint layer; uint samples; uint op; };
[[vk::binding(3, 0)]] ConstantBuffer<ResolveConstants> constants : register(b3);
[[vk::binding(0, 0)]] Texture2DMSArray<float4> image : register(t0);
[shader("fragment")]
float4 openrhi_resolve_array_0(float4 p : SV_Position) : SV_Target0 {
    uint2 q = uint2(p.xy) + constants.origin;
    float4 value = ReadSample(image, q, constants.layer, 0);

    for (uint i = 1; i < constants.samples; i++) {
        float4 sample = ReadSample(image, q, constants.layer, i);
        value = constants.op == 0 ? value + sample : (constants.op == 1 ? min(value, sample) : max(value, sample));
    }

    return constants.op == 0 ? value / float(constants.samples) : value;
}
#elif defined(OPENRHI_INTERNAL_RESOLVE_array_1)
// Slang 2026.20 emits signed MSAA coordinates; Metal requires unsigned coordinates
uint4 ReadSample(Texture2DMSArray<uint4> image, uint2 coordinate, uint layer, uint sample) {
    __intrinsic_asm "$0.read(uint2($1), uint($2), uint($3))";
}
struct ResolveConstants { uint2 origin; uint layer; uint samples; uint op; };
[[vk::binding(3, 0)]] ConstantBuffer<ResolveConstants> constants : register(b3);
[[vk::binding(0, 0)]] Texture2DMSArray<uint4> image : register(t0);
[shader("fragment")]
uint4 openrhi_resolve_array_1(float4 p : SV_Position) : SV_Target0 {
    uint2 q = uint2(p.xy) + constants.origin;
    uint4 value = ReadSample(image, q, constants.layer, 0);

    for (uint i = 1; i < constants.samples; i++) {
        uint4 sample = ReadSample(image, q, constants.layer, i);
        value = constants.op == 0 ? value + sample : (constants.op == 1 ? min(value, sample) : max(value, sample));
    }

    return constants.op == 0 ? value / uint(constants.samples) : value;
}
#elif defined(OPENRHI_INTERNAL_RESOLVE_array_2)
// Slang 2026.20 emits signed MSAA coordinates; Metal requires unsigned coordinates
int4 ReadSample(Texture2DMSArray<int4> image, uint2 coordinate, uint layer, uint sample) {
    __intrinsic_asm "$0.read(uint2($1), uint($2), uint($3))";
}
struct ResolveConstants { uint2 origin; uint layer; uint samples; uint op; };
[[vk::binding(3, 0)]] ConstantBuffer<ResolveConstants> constants : register(b3);
[[vk::binding(0, 0)]] Texture2DMSArray<int4> image : register(t0);
[shader("fragment")]
int4 openrhi_resolve_array_2(float4 p : SV_Position) : SV_Target0 {
    uint2 q = uint2(p.xy) + constants.origin;
    int4 value = ReadSample(image, q, constants.layer, 0);

    for (uint i = 1; i < constants.samples; i++) {
        int4 sample = ReadSample(image, q, constants.layer, i);
        value = constants.op == 0 ? value + sample : (constants.op == 1 ? min(value, sample) : max(value, sample));
    }

    return constants.op == 0 ? value / int(constants.samples) : value;
}
#elif defined(OPENRHI_INTERNAL_FILTER)
struct FilterDrawsArgs { uint* source; uint* count; uint* destination; uint drawNum; uint stride; uint words; };
[[vk::binding(0, 0)]] ConstantBuffer<FilterDrawsArgs> args : register(b0);
[shader("compute")][numthreads(64, 1, 1)]
void openrhi_filter_draws(uint i : SV_DispatchThreadID) {
    if (i >= args.drawNum)
        return;

    for (uint j = 0; j < args.words; j++)
        args.destination[i * args.words + j] = i < *args.count ? args.source[i * args.stride + j] : 0;
}
#elif defined(OPENRHI_INTERNAL_ROOTS)
struct PrepareDrawRootsArgs { uint* source; uint* root; uint* roots; uint drawNum; uint stride; uint rootWords; uint parameterOffset; uint indexOffset; };
[[vk::binding(0, 0)]] ConstantBuffer<PrepareDrawRootsArgs> args : register(b0);
[shader("compute")][numthreads(64, 1, 1)]
void openrhi_prepare_draw_roots(uint i : SV_DispatchThreadID) {
    if (i >= args.drawNum)
        return;

    for (uint j = 0; j < args.rootWords; j++)
        args.roots[i * args.rootWords + j] = args.root[j];

    if (args.parameterOffset != ~0u) {
        args.roots[i * args.rootWords + args.parameterOffset] = args.source[i * args.stride];
        args.roots[i * args.rootWords + args.parameterOffset + 1] = args.source[i * args.stride + 1];
    }

    if (args.indexOffset != ~0u)
        args.roots[i * args.rootWords + args.indexOffset] = i;
}
#elif defined(OPENRHI_INTERNAL_DEPTH)
[shader("fragment")]
void openrhi_depth_only() {}
#else
struct Instance { float transform[12]; uint idMask; uint offsetFlags; uint64_t accelerationStructure; };
struct MetalInstance { float transform[12]; uint options; uint mask; uint intersectionFunctionTableOffset; uint userID; uint64_t accelerationStructure; };
struct TopLevelHeader { uint64_t accelerationStructure; uint* instanceContributions; uint64_t reserved[6]; };
[ForceInline]
void WriteTopLevelHeader(TopLevelHeader* header, uint64_t accelerationStructure) {
    header->accelerationStructure = accelerationStructure;
    header->instanceContributions = (uint*)(header + 1);

    for (uint i = 0; i < 6; i++)
        header->reserved[i] = 0;
}
#if defined(OPENRHI_INTERNAL_INSTANCES)
struct ConvertInstancesArgs { Instance* src; MetalInstance* dst; TopLevelHeader* header; uint64_t accelerationStructure; uint instanceNum; };
[[vk::binding(3, 0)]] ConstantBuffer<ConvertInstancesArgs> args : register(b3);
[shader("compute")][numthreads(64, 1, 1)]
void openrhi_convert_instances(uint i : SV_DispatchThreadID) {
    if (i == 0)
        WriteTopLevelHeader(args.header, args.accelerationStructure);

    if (i >= args.instanceNum)
        return;

    Instance source = args.src[i];
    MetalInstance destination;

    for (uint j = 0; j < 12; j++)
        destination.transform[j] = source.transform[j];
    destination.options = (source.offsetFlags >> 24) & 0xf;
    destination.mask = source.idMask >> 24;
    destination.intersectionFunctionTableOffset = 0;
    destination.userID = source.idMask & 0xffffff;
    destination.accelerationStructure = source.accelerationStructure;
    args.dst[i] = destination;
    uint* contributions = (uint*)(args.header + 1);
    contributions[i] = source.offsetFlags & 0xffffff;
}
#elif defined(OPENRHI_INTERNAL_HEADER)
struct CopyTopLevelHeaderArgs { uint* srcContributions; TopLevelHeader* dst; uint64_t dstAccelerationStructure; uint num; };
[[vk::binding(3, 0)]] ConstantBuffer<CopyTopLevelHeaderArgs> args : register(b3);
[shader("compute")][numthreads(64, 1, 1)]
void openrhi_copy_top_level_header(uint i : SV_DispatchThreadID) {
    if (i == 0)
        WriteTopLevelHeader(args.dst, args.dstAccelerationStructure);

    if (i < args.num)
        ((uint*)(args.dst + 1))[i] = args.srcContributions[i];
}
#elif defined(OPENRHI_INTERNAL_WORDS)
struct CopyWordsArgs { uint* src; uint* dst; };
[[vk::binding(3, 0)]] ConstantBuffer<CopyWordsArgs> args : register(b3);
[shader("compute")][numthreads(64, 1, 1)]
void openrhi_copy_words(uint i : SV_DispatchThreadID) { args.dst[i] = args.src[i]; }
#endif
#endif
