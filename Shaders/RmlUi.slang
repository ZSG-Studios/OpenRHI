// OpenRHI RmlUi renderer: premultiplied RGBA and column-major transforms
#include "OpenRHI.slang"

struct RmlUiConstants {
    float4 column0;
    float4 column1;
    float4 column2;
    float4 column3;
    float2 invDisplay;
    float2 translation;
    float hdrScale;
    uint linearColor;
    float2 padding;
};
struct RmlUiVertexIn {
    [[vk::location(0)]] float2 position : POSITION0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
};
struct RmlUiVertexOut {
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};
#if OPENRHI_TARGET_METAL
struct RmlUiRoot {
    RmlUiConstants constants;
    OpenRhiSamplerEntry* samplers;
    OpenRhiTexture2DEntry* textures;
};
[[vk::binding(2, 0)]] ConstantBuffer<RmlUiRoot> root : register(b2);
#define constants root.constants
#define linearClamp root.samplers[0].sampler
#define image root.textures[0].texture
#else
OPENRHI_ROOT_CONSTANTS(RmlUiConstants, constants, 0, 0);
OPENRHI_RESOURCE(SamplerState, linearClamp, s, 1, 0);
OPENRHI_RESOURCE(Texture2D<float4>, image, t, 0, 1);
#endif

[shader("vertex")]
RmlUiVertexOut RmlUiVS(RmlUiVertexIn input) {
    float2 p = input.position + constants.translation;
    float4 transformed = constants.column0 * p.x + constants.column1 * p.y + constants.column3;
    RmlUiVertexOut output;
    float2 uv = transformed.xy * constants.invDisplay;
    output.position = float4(uv.x * 2 - transformed.w, transformed.w - uv.y * 2, 0, transformed.w);
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

[shader("fragment")]
float4 RmlUiFS(RmlUiVertexOut input) : SV_Target0 {
    float4 color = input.color * image.Sample(linearClamp, input.uv);
    if (constants.linearColor != 0 && color.a > 0) {
        float3 straight = saturate(color.rgb / color.a);
        float3 low = straight / 12.92;
        float3 high = pow((straight + 0.055) / 1.055, 2.4);
        color.rgb = select(straight <= 0.04045, low, high) * color.a;
    }
    color.rgb *= constants.hdrScale;

    return color;
}
