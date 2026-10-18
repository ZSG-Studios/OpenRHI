// © 2025 NVIDIA Corporation

#include "OpenRHI.slang"

#define NIS_NV12_SUPPORT 0
#define NIS_SCALER 1
#define NIS_CLAMP_OUTPUT 1
#define NIS_VIEWPORT_SUPPORT 1
#define NIS_BLOCK_WIDTH 32
#define kContrastBoost 1.0
#define NIS_USE_HALF_PRECISION NIS_FP16
#if OPENRHI_TARGET_METAL
// Keep instrumented threadgroup storage within the native 32 KiB limit
#if NIS_FP16
#define NIS_BLOCK_HEIGHT 24
#else
#define NIS_BLOCK_HEIGHT 16
#endif
#elif NIS_FP16
#define NIS_BLOCK_HEIGHT 32
#else
#define NIS_BLOCK_HEIGHT 24
#endif

struct Constants { // see NIS.h
    float detectRatio;
    float detectThres;
    float minContrastRatio;
    float ratioNorm;

    float sharpStartY;
    float sharpScaleY;
    float sharpStrengthMin;
    float sharpStrengthScale;

    float sharpLimitMin;
    float sharpLimitScale;
    float scaleX;
    float scaleY;

    float dstNormX;
    float dstNormY;
    float srcNormX;
    float srcNormY;

#if (NIS_VIEWPORT_SUPPORT == 1)
    uint inputViewportW;
    uint inputViewportH;
    uint outputViewportW;
    uint outputViewportH;
#endif
};

#define OPENRHI_NIS_SET_STATIC              0
#define OPENRHI_NIS_SET_DYNAMIC             1

#if NIS_OUTPUT_FORMAT == 0
    #define NIS_OUTPUT_FORMAT_ATTRIBUTE "unknown"
#elif NIS_OUTPUT_FORMAT == 1
    #define NIS_OUTPUT_FORMAT_ATTRIBUTE "rgba8"
#elif NIS_OUTPUT_FORMAT == 2
    #define NIS_OUTPUT_FORMAT_ATTRIBUTE "rgba16f"
#elif NIS_OUTPUT_FORMAT == 3
    #define NIS_OUTPUT_FORMAT_ATTRIBUTE "rgba32f"
#else
    #error Unsupported NIS output format
#endif

// Shader

#define kDetectRatio                    constants.detectRatio
#define kDetectThres                    constants.detectThres
#define kMinContrastRatio               constants.minContrastRatio
#define kRatioNorm                      constants.ratioNorm
#define kSharpStartY                    constants.sharpStartY
#define kSharpScaleY                    constants.sharpScaleY
#define kSharpStrengthMin               constants.sharpStrengthMin
#define kSharpStrengthScale             constants.sharpStrengthScale
#define kSharpLimitMin                  constants.sharpLimitMin
#define kSharpLimitScale                constants.sharpLimitScale
#define kScaleX                         constants.scaleX
#define kScaleY                         constants.scaleY
#define kDstNormX                       constants.dstNormX
#define kDstNormY                       constants.dstNormY
#define kSrcNormX                       constants.srcNormX
#define kSrcNormY                       constants.srcNormY

#if (NIS_VIEWPORT_SUPPORT == 1)
    #define kInputViewportWidth         constants.inputViewportW
    #define kInputViewportHeight        constants.inputViewportH
    #define kOutputViewportWidth        constants.outputViewportW
    #define kOutputViewportHeight       constants.outputViewportH
#endif

#if OPENRHI_TARGET_METAL
struct NisDynamicEntry {
    uint64_t bufferAddress;
    DescriptorHandle<RWTexture2D<float4>> texture;
    uint64_t metadata;
};
struct NisRoot {
    Constants constants;
    OpenRhiTexture2DEntry* coefficients;
    OpenRhiSamplerEntry* samplers;
    OpenRhiTexture2DEntry* images;
};
[[vk::binding(2, 0)]] ConstantBuffer<NisRoot> root : register(b2);
#define constants root.constants
#define samplerLinearClamp root.samplers[0].sampler
#define coef_scaler root.coefficients[0].texture
#define coef_usm root.coefficients[1].texture
#define in_texture root.images[0].texture
#define out_texture ((NisDynamicEntry*)root.images)[1].texture
#else
OPENRHI_ROOT_CONSTANTS(Constants, constants,           0, OPENRHI_NIS_SET_STATIC);
OPENRHI_RESOURCE(SamplerState, samplerLinearClamp,  s, 1, OPENRHI_NIS_SET_STATIC);
OPENRHI_RESOURCE(Texture2D<float4>, coef_scaler,    t, 2, OPENRHI_NIS_SET_STATIC);
OPENRHI_RESOURCE(Texture2D<float4>, coef_usm,       t, 3, OPENRHI_NIS_SET_STATIC);

OPENRHI_RESOURCE(Texture2D<float4>, in_texture,     t, 0, OPENRHI_NIS_SET_DYNAMIC);
OPENRHI_FORMAT(NIS_OUTPUT_FORMAT_ATTRIBUTE) OPENRHI_RESOURCE(RWTexture2D<float4>, out_texture,  u, 1, OPENRHI_NIS_SET_DYNAMIC);

#endif

// https://github.com/NVIDIAGameWorks/NVIDIAImageScaling/blob/main/NIS/NIS_Scaler.h
// Add this line in "CalcLTI" function:
//    const float kEps = 1.0 / 255.0;
#include "NisScaler.slang"

[shader("compute")]
[numthreads(NIS_THREAD_GROUP_SIZE, 1, 1)]
void NisMain(uint3 blockIdx : SV_GroupID, uint3 threadIdx : SV_GroupThreadID) {
    NVScaler(blockIdx.xy, threadIdx.x);
}
