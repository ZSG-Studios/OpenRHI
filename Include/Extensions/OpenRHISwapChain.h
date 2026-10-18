// © 2021 NVIDIA Corporation

// Goal: presentation functionality

#pragma once

#define OPENRHI_SWAP_CHAIN_H 1

OpenRhiNamespaceBegin

OpenRhiForwardStruct(SwapChain);

// Special "initialValue" for "CreateFence" needed to create swap chain related semaphores
static const uint64_t OpenRhiConstant(SWAPCHAIN_SEMAPHORE) = (uint64_t)(-1);

// Color space:
//  - BT.709 - LDR https://en.wikipedia.org/wiki/Rec._709
//  - BT.2020 - HDR https://en.wikipedia.org/wiki/Rec._2020
//  - https://registry.khronos.org/DataFormat/specs/1.4/dataformat.1.4.html#PRIMARY_CONVERSION
// Transfer function:
//  - G10 - linear (gamma 1.0)
//  - G22 - sRGB (gamma ~2.2)
//  - G2084 - SMPTE ST.2084 (Perceptual Quantization)
//  - https://registry.khronos.org/DataFormat/specs/1.4/dataformat.1.4.html#TRANSFER_CONVERSION
// Bits per channel:
//  - 8, 10, 16 (float)
OpenRhiEnum(SwapChainFormat, uint8_t,
    BT709_G10_16BIT,
    BT709_G22_8BIT,
    BT709_G22_10BIT,
    BT2020_G2084_10BIT
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPresentScalingFlagBitsKHR.html
OpenRhiEnum(Scaling, uint8_t,
    ONE_TO_ONE,                      // no scaling, 1:1 pixel mapping
    STRETCH                          // minified or magnified stretching
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPresentGravityFlagBitsKHR.html
OpenRhiEnum(Gravity, uint8_t,
    MIN,                             // pixels will gravitate towards the top or left side of the surface (Windows style)
    MAX,                             // VK: pixels will gravitate towards the bottom or right side of the surface
    CENTERED                         // VK: pixels will be centered in the surface
);

OpenRhiBits(SwapChainBits, uint8_t,
    NONE                = 0,
    VSYNC               = OpenRhiBit(0), // cap framerate to the monitor refresh rate
    WAITABLE            = OpenRhiBit(1), // unlock "WaitForPresent" reducing latency (requires "features.waitableSwapChain")
    ALLOW_TEARING       = OpenRhiBit(2), // allow screen tearing if possible (METAL: "displaySyncEnabled = NO", may show partially rendered frames)
    ALLOW_LOW_LATENCY   = OpenRhiBit(3)  // allow "LowLatencyInterface" functionality (requires "features.lowLatency")
);

OpenRhiStruct(WindowsWindow) {  // Expects "WIN32" platform macro
    void* hwnd;             //    HWND
};

OpenRhiStruct(X11Window) {      // Expects "OPENRHI_ENABLE_XLIB_SUPPORT"
    void* dpy;              //    Display
    uint64_t window;        //    Window
};

OpenRhiStruct(WaylandWindow) {  // Expects "OPENRHI_ENABLE_WAYLAND_SUPPORT"
    void* display;          //    wl_display
    void* surface;          //    wl_surface
};

OpenRhiStruct(MetalWindow) {    // Expects "APPLE" platform macro
    void* caMetalLayer;     //    CAMetalLayer
};

OpenRhiStruct(AndroidWindow) {  // Expects "__ANDROID__" platform macro
    void* nativeWindow;     //    ANativeWindow
};

OpenRhiStruct(Window) {
    // Only one entity must be initialized
    OpenRhi(WindowsWindow) windows;
    OpenRhi(X11Window) x11;
    OpenRhi(WaylandWindow) wayland;
    OpenRhi(MetalWindow) metal;
    OpenRhi(AndroidWindow) android;
};

// SwapChain textures will be created as "color attachment" resources
// queuedFrameNum = 0 - auto-selection between 1 (for waitable) or 2 (otherwise)
// queuedFrameNum = 2 - recommended if the GPU frame time is less than the desired frame time, but the sum of 2 frames is greater
OpenRhiStruct(SwapChainDesc) {
    OpenRhi(Window) window;
    const OpenRhiPtr(Queue) queue;                  // GRAPHICS or COMPUTE (requires "features.presentFromCompute")
    OpenRhi(Dim_t) width;
    OpenRhi(Dim_t) height;
    uint8_t textureNum;                         // desired value, real value must be queried using "GetSwapChainTextures"
    OpenRhi(SwapChainFormat) format;                // desired format, real value must be queried using "GetTextureDesc" for one of the swap chain textures
    OpenRhi(SwapChainBits) flags;
    OpenRhiOptional uint8_t queuedFrameNum;         // aka "max frame latency", aka "number of frames in flight"

    // Present scaling and positioning, silently ignored if "features.resizableSwapChain" is not supported or not supported by the implicitly choosen present mode
    OpenRhiOptional OpenRhi(Scaling) scaling;           // VK: if scaling is not supported, "OUT_OF_DATE" error is triggered on resizing
    OpenRhiOptional OpenRhi(Gravity) gravityX;
    OpenRhiOptional OpenRhi(Gravity) gravityY;
};

OpenRhiStruct(ChromaticityCoords) {
    float x, y; // [0; 1]
};

// Describes color settings and capabilities of the closest display:
//  - Luminance provided in nits (cd/m2)
//  - SDR = standard dynamic range
//  - LDR = low dynamic range (in many cases LDR == SDR)
//  - HDR = high dynamic range, assumes G2084:
//      - BT709_G10_16BIT: HDR gets enabled and applied implicitly if Windows HDR is enabled
//      - BT2020_G2084_10BIT: HDR requires explicit color conversions and enabled HDR in Windows
//  - "SDR scale in HDR mode" = sdrLuminance / 80
OpenRhiStruct(DisplayDesc) {
    OpenRhi(ChromaticityCoords) redPrimary;
    OpenRhi(ChromaticityCoords) greenPrimary;
    OpenRhi(ChromaticityCoords) bluePrimary;
    OpenRhi(ChromaticityCoords) whitePoint;
    float minLuminance;
    float maxLuminance;
    float maxFullFrameLuminance;
    float sdrLuminance;
    bool isHDR;
};

// Threadsafe: yes
OpenRhiStruct(SwapChainInterface) {
    OpenRhi(Result)             (OPENRHI_CALL *CreateSwapChain)         (OpenRhiRef(Device) device, const OpenRhiRef(SwapChainDesc) swapChainDesc, OpenRhiOut OpenRhiRef(SwapChain*) swapChain);
    void                    (OPENRHI_CALL *DestroySwapChain)        (OpenRhiPtr(SwapChain) swapChain);
    OpenRhiPtr(Texture) const*  (OPENRHI_CALL *GetSwapChainTextures)    (const OpenRhiRef(SwapChain) swapChain, OpenRhiOut NonOpenRhiRef(uint32_t) textureNum);

    // Returns "FAILURE" if swap chain's window is outside of all monitors
    OpenRhi(Result)             (OPENRHI_CALL *GetDisplayDesc)          (OpenRhiRef(SwapChain) swapChain, OpenRhiOut OpenRhiRef(DisplayDesc) displayDesc);

    // "acquireSemaphore" must be created with "SWAPCHAIN_SEMAPHORE" initial value (ignored in D3D). VK only: may return "OUT_OF_DATE"
    OpenRhi(Result)             (OPENRHI_CALL *AcquireNextTexture)      (OpenRhiRef(SwapChain) swapChain, OpenRhiRef(Fence) acquireSemaphore, OpenRhiOut NonOpenRhiRef(uint32_t) textureIndex);

    // "presentId" must identify a previously queued presentation. Call once immediately before input sampling
    OpenRhi(Result)             (OPENRHI_CALL *WaitForPresent)          (OpenRhiRef(SwapChain) swapChain, uint64_t presentId);

    // A non-zero "presentId" associates the presentation with a tracked frame. Use 0 for an untracked presentation.
    // Non-zero "presentId" values must strictly increase across frames for each swap chain, i.e. gaps are allowed
    // "releaseSemaphore" must be created with "SWAPCHAIN_SEMAPHORE" initial value (ignored in D3D)
    OpenRhi(Result)             (OPENRHI_CALL *QueuePresent)            (OpenRhiRef(SwapChain) swapChain, OpenRhiRef(Fence) releaseSemaphore, uint64_t presentId);
};

/*
Typical usage example, valid if the number of swap chain images >= queued frames:

// Creation:
    // Create swap chain
        SwapChainDesc swapChainDesc = {};
        swapChainDesc.textureNum = QUEUED_FRAME_NUM + 1; // assuming no correction after swap chain creation for simplicity
        ...

    // Create in-flight data
        struct SwapChainTexture {
            Fence* acquireSemaphore;
            Fence* releaseSemaphore;
            ...
        } swapChainTextures[16] = {};

        Fence* frameFence;
        OpenRHI.CreateFence(device, 0, frameFence);

        // Use "GetSwapChainTextures" to populate
        for (uint32_t i = 0; i < swapChainDesc.textureNum; i++) {
            OpenRHI.CreateFence(device, SWAPCHAIN_SEMAPHORE, swapChainTextures[i].swapChainAcquireSemaphore);
            OpenRHI.CreateFence(device, SWAPCHAIN_SEMAPHORE, swapChainTextures[i].swapChainReleaseSemaphore);
            ...
        }

// Rendering:
    // Wait for the tail of enqueued frames
        OpenRHI.Wait(frameFence, frameIndex >= QUEUED_FRAME_NUM ? 1 + frameIndex - QUEUED_FRAME_NUM : 0);

    // Acquire a swap chain texture
        uint32_t recycledSemaphoreIndex = frameIndex % swapChainDesc.textureNum;
        Fence* acquireSemaphore = swapChainTextures[recycledSemaphoreIndex].acquireSemaphore;

        uint32_t currentSwapChainTextureIndex = 0;
        OpenRHI.AcquireNextTexture(swapChain, *acquireSemaphore, currentSwapChainTextureIndex);

    // Record command buffers
        ... // use swap chain texture with index "currentSwapChainTextureIndex"

    // Submit
        Fence* releaseSemaphore = swapChainTextures[currentSwapChainTextureIndex].releaseSemaphore;

        FenceSubmitDesc frameFence = {};
        frameFence.fence = m_FrameFence;
        frameFence.value = 1 + frameIndex;

        FenceSubmitDesc textureAcquiredFence = {};
        textureAcquiredFence.fence = acquireSemaphore;
        textureAcquiredFence.stages = COLOR_ATTACHMENT; // COPY, ALL

        FenceSubmitDesc renderingFinishedFence = {};
        renderingFinishedFence.fence = releaseSemaphore;

        FenceSubmitDesc signalFences[] = {renderingFinishedFence, frameFence};

        QueueSubmitDesc queueSubmitDesc = {};
        queueSubmitDesc.waitFences = &textureAcquiredFence;
        queueSubmitDesc.waitFenceNum = 1;
        queueSubmitDesc.commandBuffers = ...;
        queueSubmitDesc.commandBufferNum = ...;
        queueSubmitDesc.signalFences = signalFences;
        queueSubmitDesc.signalFenceNum = 2;

        OpenRHI.QueueSubmit(queue, queueSubmitDesc);

    // Present
        OpenRHI.QueuePresent(swapChain, *releaseSemaphore, 1 + frameIndex);
*/

OpenRhiNamespaceEnd
