// © 2025 NVIDIA Corporation

// Goal: providing easy-to-use access to modern upscalers: DLSS, FSR, XESS, NIS, MetalFX

#pragma once

#define OPENRHI_UPSCALER_H 1

OpenRhiNamespaceBegin

OpenRhiForwardStruct(Upscaler);

OpenRhiEnum(UpscalerType, uint8_t,  // Name                                     // Notes
    NIS,                        // NVIDIA Image Scaling                     sharpener-upscaler, cross vendor
    FSR,                        // AMD FidelityFX Super Resolution          FSR 4 where supported, FSR 3 otherwise
    XESS,                       // INTEL XeSS Super Resolution              upscaler, cross vendor
    DLSR,                       // NVIDIA Deep Learning Super Resolution    upscaler, NVIDIA only
    DLRR,                       // NVIDIA Deep Learning Ray Reconstruction  upscaler-denoiser, NVIDIA only
    METALFX_SPATIAL,            // Apple MetalFX spatial scaler             upscaler, Metal only
    METALFX_TEMPORAL,           // Apple MetalFX temporal scaler            upscaler, Metal only
    METALFX_DENOISED            // Apple MetalFX temporal denoised scaler   upscaler-denoiser, Metal only
);

// MetalFX: created on the first dispatch (a hitch), resource sizes and formats must not change after it. "output" needs "COLOR_ATTACHMENT" usage.
// No "DEPTH_LINEAR" and "MV_JITTERED" ("MV_UPSCALED" requires macOS 27). "METALFX_DENOISED": no "MV_UPSCALED", "sss" and "preTransparency", "specularMvOrHitT" is "hit distance"

OpenRhiEnum(UpscalerMode, uint8_t,  // Scaling factor       // Min jitter phases (or just use unclamped Halton2D)
    NATIVE,                     // 1.0x                 8
    ULTRA_QUALITY,              // 1.3x                 14
    QUALITY,                    // 1.5x                 18
    BALANCED,                   // 1.7x                 23
    PERFORMANCE,                // 2.0x                 32
    ULTRA_PERFORMANCE           // 3.0x                 72
);

OpenRhiBits(UpscalerBits, uint16_t,
    NONE                        = 0,
    HDR                         = OpenRhiBit(0),            // "input" uses colors in High-Dynamic Range (HDR)
    SRGB                        = OpenRhiBit(1),            // "input" uses Low-Dynamic Range (LDR) colors in sRGB space
    USE_EXPOSURE                = OpenRhiBit(2),            // "exposure" texture is provided (automatic exposure otherwise)
    USE_REACTIVE                = OpenRhiBit(3),            // "reactive" texture is provided
    DEPTH_INVERTED              = OpenRhiBit(4),            // "depth" is inverted, i.e. the near plane is mapped to 1
    DEPTH_INFINITE              = OpenRhiBit(5),            // "depth" uses INF far plane
    DEPTH_LINEAR                = OpenRhiBit(6),            // "depth" is linear viewZ (HW otherwise)
    MV_UPSCALED                 = OpenRhiBit(7),            // "mv" are rendered at upscale resolution
    MV_JITTERED                 = OpenRhiBit(8)             // "mv" include jitter
);

OpenRhiBits(DispatchUpscaleBits, uint8_t,
    NONE                        = 0,
    RESET_HISTORY               = OpenRhiBit(0),            // restart accumulation
    USE_SPECULAR_MOTION         = OpenRhiBit(1)             // ("DLRR" only) if set, "specularMvOrHitT" represents "specular motion" not "hit distance"
);

OpenRhiStruct(UpscalerDesc) {
    OpenRhi(Dim2_t) upscaleResolution;                      // output resolution
    OpenRhi(UpscalerType) type;
    OpenRhi(UpscalerMode) mode;                             // not needed for NIS
    OpenRhi(UpscalerBits) flags;
    OpenRhiOptional uint8_t preset;                         // preset for DLSR or XESS (0 default, >1 presets A, B, C...)
    OpenRhiOptional OpenRhi(Format) outputFormat;               // needed only for NIS, if "shaderFeatures.storageWriteWithoutFormat" is unsupported
    OpenRhiOptional OpenRhiPtr(CommandBuffer) commandBuffer;    // a non-copy-only command buffer in opened state, submission must be done manually ("wait for idle" executed, if not provided)
};

OpenRhiStruct(UpscalerProps) {
    float scalingFactor;                                // per dimension scaling factor
    float mipBias;                                      // mip bias for materials textures, computed as "-log2(scalingFactor) - 1" (keep an eye on normal maps)
    OpenRhi(Dim2_t) upscaleResolution;                      // output resolution
    OpenRhi(Dim2_t) renderResolution;                       // optimal render resolution
    OpenRhi(Dim2_t) renderResolutionMin;                    // minimal render resolution (for Dynamic Resolution Scaling)
    uint8_t jitterPhaseNum;                             // minimal number of phases in the jitter sequence, computed as "ceil(8 * scalingFactor ^ 2)" ("Halton(2, 3)" recommended)
};

OpenRhiStruct(UpscalerResource) {
    OpenRhiPtr(Texture) texture;
    OpenRhiPtr(Descriptor) descriptor;                      // "SHADER_RESOURCE" or "SHADER_RESOURCE_STORAGE", see comments below
};

// Guide buffers
OpenRhiStruct(UpscalerGuides) {                             // For FSR, XESS, DLSR, METALFX_TEMPORAL
    OpenRhi(UpscalerResource) mv;                           // .xy - surface motion
    OpenRhi(UpscalerResource) depth;                        // .x - HW depth
    OpenRhiOptional OpenRhi(UpscalerResource) exposure;         // .x - 1x1 exposure
    OpenRhiOptional OpenRhi(UpscalerResource) reactive;         // .x - bias towards "input"
};

OpenRhiStruct(DenoiserGuides) {                             // For DLRR, METALFX_DENOISED
    OpenRhi(UpscalerResource) mv;                           // .xy - surface motion
    OpenRhi(UpscalerResource) depth;                        // .x - HW or linear depth
    OpenRhi(UpscalerResource) normalRoughness;              // .xyz - world-space normal (not encoded), .w - linear roughness
    OpenRhi(UpscalerResource) diffuseAlbedo;                // .xyz - diffuse albedo (LDR sky color for sky)
    OpenRhi(UpscalerResource) specularAlbedo;               // .xyz - specular albedo (environment BRDF)
    OpenRhi(UpscalerResource) specularMvOrHitT;             // .xy - specular virtual motion of the reflected world, or .x - specular hit distance otherwise
    OpenRhiOptional OpenRhi(UpscalerResource) exposure;         // .x - 1x1 exposure
    OpenRhiOptional OpenRhi(UpscalerResource) reactive;         // .x - bias towards "input"
    OpenRhiOptional OpenRhi(UpscalerResource) preTransparency;  // .xyz - color before transparency (aka transparency guide)
    OpenRhiOptional OpenRhi(UpscalerResource) sss;              // .x - subsurface scattering, computed as "Luminance(colorAfterSSS - colorBeforeSSS)"
};

// Settings
OpenRhiStruct(NISSettings) {
    float sharpness;                                    // [0; 1]
};

OpenRhiStruct(FSRSettings) {
    float zNear;                                        // distance to the near plane (units)
    float zFar;                                         // distance to the far plane, unused if "DEPTH_INFINITE" is set (units)
    float verticalFov;                                  // vertical field of view angle (radians)
    float frameTime;                                    // the time elapsed since the last frame (ms)
    float viewSpaceToMetersFactor;                      // for converting view space units to meters (m/unit)
    float sharpness;                                    // [0; 1]
};

OpenRhiStruct(DLRRSettings) {
    float worldToViewMatrix[16];                        // {Xx, Yx, Zx, 0, Xy, Yy, Zy, 0, Xz, Yz, Zz, 0, Tx, Ty, Tz, 1}, where {X, Y, Z} - axises, T - translation
    float viewToClipMatrix[16];                         // {-, -, -, 0, -, -, -, 0, -, -, -, A, -, -, -, B}, where {A; B} = {0; 1} for ortho or {-1/+1; 0} for perspective projections
};

OpenRhiStruct(DispatchUpscaleDesc) {
    // Output (required "SHADER_RESOURCE_STORAGE" for resource state & descriptor)
    OpenRhi(UpscalerResource) output;                       // .xyz - upscaled RGB color

    // Input (required "SHADER_RESOURCE" for resource state & descriptor)
    OpenRhi(UpscalerResource) input;                        // .xyz - input RGB color

    // Guides (required "SHADER_RESOURCE" for resource states & descriptors)
    union {                                             // Choosen based on "UpscalerType" passed during creation
        OpenRhi(UpscalerGuides) upscaler;                   //      FSR, XESS, DLSR, METALFX_TEMPORAL
        OpenRhi(DenoiserGuides) denoiser;                   //      DLRR, METALFX_DENOISED (sRGB not supported)
    } guides;

    // Settings
    union {                                             // Choosen based on "UpscalerType" passed during creation
        OpenRhi(NISSettings) nis;                           //      NIS settings
        OpenRhi(FSRSettings) fsr;                           //      FSR settings
        OpenRhi(DLRRSettings) dlrr;                         //      DLRR and METALFX_DENOISED settings
    } settings;

    OpenRhi(Dim2_t) currentResolution;                      // current render resolution for inputs and guides, renderResolutionMin <= currentResolution <= renderResolution
    OpenRhi(Float2_t) cameraJitter;                         // pointing towards the pixel center, in [-0.5; 0.5] range
    OpenRhi(Float2_t) mvScale;                              // used to convert motion vectors to pixel space
    OpenRhiOptional float preExposure;                      // multiplier already applied to the "input" color, 0 means 1 (FSR, XESS, DLSR, DLRR, METALFX_TEMPORAL, METALFX_DENOISED)
    OpenRhi(DispatchUpscaleBits) flags;
};

// Threadsafe: yes
OpenRhiStruct(UpscalerInterface) {
    OpenRhi(Result)     (OPENRHI_CALL *CreateUpscaler)          (OpenRhiRef(Device) device, const OpenRhiRef(UpscalerDesc) upscalerDesc, OpenRhiOut OpenRhiRef(Upscaler*) upscaler);
    void            (OPENRHI_CALL *DestroyUpscaler)         (OpenRhiPtr(Upscaler) upscaler);

    bool            (OPENRHI_CALL *IsUpscalerSupported)     (const OpenRhiRef(Device) device, OpenRhi(UpscalerType) type);
    void            (OPENRHI_CALL *GetUpscalerProps)        (const OpenRhiRef(Upscaler) upscaler, OpenRhiOut OpenRhiRef(UpscalerProps) upscalerProps);

    // Command buffer
    // {
        // Dispatch (changes descriptor pool, pipeline layout and pipeline, barriers are externally controlled)
        void        (OPENRHI_CALL *CmdDispatchUpscale)      (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(Upscaler) upscaler, const OpenRhiRef(DispatchUpscaleDesc) dispatchUpscaleDesc);
    // }
};

OpenRhiNamespaceEnd