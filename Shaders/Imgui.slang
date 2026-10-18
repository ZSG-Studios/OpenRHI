// © 2025-2026 NVIDIA Corporation
#include "OpenRHI.slang"

struct ImguiConstants {
    float invDisplayWidth;
    float invDisplayHeight;
    float hdrScale;
    float gamma;
};
struct ImguiVertexIn {
    [[vk::location(0)]] float2 position : POSITION0;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
    [[vk::location(2)]] float4 color : COLOR0;
};
struct ImguiVertexOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};
#if OPENRHI_TARGET_METAL
struct ImguiRoot {
    ImguiConstants constants;
    OpenRhiSamplerEntry* samplers;
    OpenRhiTexture2DEntry* textures;
};
[[vk::binding(2, 0)]] ConstantBuffer<ImguiRoot> root : register(b2);
#define constants root.constants
#define linearClamp root.samplers[0].sampler
#define image root.textures[0].texture
#else
OPENRHI_ROOT_CONSTANTS(ImguiConstants, constants, 0, 0);
OPENRHI_RESOURCE(SamplerState, linearClamp, s, 1, 0);
OPENRHI_RESOURCE(Texture2D<float4>, image, t, 0, 1);
#endif

[shader("vertex")]
ImguiVertexOut ImguiVS(ImguiVertexIn input) {
    ImguiVertexOut output;
    float2 uv = input.position * float2(constants.invDisplayWidth, constants.invDisplayHeight);
    output.position = float4(OPENRHI_UV_TO_CLIP(uv), 0, 1);
    output.uv = input.uv;
    output.color = input.color;
    output.color.rgb = pow(saturate(output.color.rgb), constants.gamma) * constants.hdrScale;

    return output;
}

[shader("fragment")]
float4 ImguiFS(ImguiVertexOut input) : SV_Target0 {
    return input.color * image.Sample(linearClamp, input.uv);
}
