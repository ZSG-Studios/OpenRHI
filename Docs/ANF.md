# Adreno Neural Fusion SR integration

OpenRHI exposes Qualcomm ANF temporal Super Resolution through `UpscalerInterface` and `UpscalerType::ANF`. The adapter uses the unmodified ANF SDK 0.43.0, commit `a144fbab47ad586adf64c5a1574be73064550f1e`, on Android ARM64/Vulkan. The current SDK's Performance mode reconstructs at 2x per dimension.

## Build and deployment

Obtain the [pinned public SDK](https://github.com/SnapdragonGameStudios/adreno-neural-fusion/tree/a144fbab47ad586adf64c5a1574be73064550f1e), then supply its root when configuring OpenRHI:

```sh
xmake f -P . -p android -a arm64-v8a --ndk="$ANDROID_NDK_HOME" --ndk_sdkver=33 -m release --anf=y --anf_root=/path/to/adreno-neural-fusion -y
xmake build -P . -y OpenRHI
xmake sdk -P .
```

Builds check the pinned runtime's SHA-256. The output and prepared SDK include `libanf.so` and both original licenses under `ThirdPartyLicenses/anf`. Package the runtime at `lib/arm64-v8a/libanf.so` in the application's APK/AAB and retain the license files. Build every application native library with compatible 16-KiB ELF segment alignment. For Android system images exposing OpenCL as a public native library, add this inside the manifest's application element:

```xml
<uses-native-library android:name="libOpenCL.so" android:required="false" />
```

Use the current public headers and rebuild consumers against API revision 1.

## Device and resource requirements

Include `OpenRHI.h`, `Extensions/OpenRHIDeviceCreation.h`, `Extensions/OpenRHIUpscaler.h`, `Extensions/OpenRHIUpscalerVK.h`, and `Extensions/OpenRHIWrapperVK.h`. Query requirements before Vulkan instance/device creation:

```cpp
openrhi::UpscalerRequirementsVK requirements = {};
openrhi::Result result = openrhi::openrhiGetUpscalerRequirementsVK(
    openrhi::UpscalerType::ANF, requirements);
```

Proceed when the query succeeds. Its extension names, resource format lists, usage flags, external-memory handle types and queue flags are copied into caller-owned storage. Convert the extension-name arrays into pointer arrays and pass them through `DeviceCreationDesc::vkExtensions`; imported Vulkan devices must already enable them. Choose a dispatch queue family satisfying every returned queue flag.

After device creation, `DeviceDesc::features.adrenoNeuralFusion` and `IsUpscalerSupported(device, UpscalerType::ANF)` report native SDK eligibility with the enabled extensions. Always check `CreateUpscaler`: technique creation can still return `UNSUPPORTED` or another error. Select a working alternative such as NIS when creation is rejected.

The resource array is indexed by `UpscalerResourceRole`:

| Role | Dimensions | Resource state at dispatch |
|---|---|---|
| `INPUT_COLOR` | Render resolution | Shader resource / `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` |
| `OUTPUT_COLOR` | Twice render width and height | Shader resource storage / `VK_IMAGE_LAYOUT_GENERAL` |
| `MOTION_VECTORS` | Render resolution | Shader resource / `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` |
| `DEPTH` | Render resolution | Shader resource / `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` |

Create single-mip, single-layer, single-sample 2D images using a returned format and all returned native usage flags. When `externalMemoryHandleTypes` is nonzero, create/bind exportable native memory with those types, then wrap the native images through `WrapperVKInterface`. Keep images, memory and descriptors alive through GPU completion. `Tests/AndroidDeviceLab/AnfDispatch.cpp` demonstrates requirement-driven native image creation, wrapping, transitions, submission and readback.

## Create, dispatch and destroy

Create outside the frame loop:

```cpp
openrhi::UpscalerDesc desc = {};
desc.type = openrhi::UpscalerType::ANF;
desc.mode = openrhi::UpscalerMode::PERFORMANCE;
desc.upscaleResolution = {1920, 1080};
desc.maxInFlight = 2;
openrhi::Upscaler* upscaler = nullptr;
result = upscalers.CreateUpscaler(*device, desc, upscaler);
```

Use even output dimensions and `UpscalerBits::NONE`. Creation fixes the input/output dimensions. Size `maxInFlight` for the maximum overlapping ANF submissions; zero selects one. Recreate after changing dimensions or the overlap limit, once previous work completes.

For each frame, populate `DispatchUpscaleDesc` with color/output resources and the upscaler depth/motion guides. Render color and depth with the supplied `cameraJitter` in pixel units within `[-0.5, 0.5]`. Supply native SDK motion vectors with projection jitter removed, `mvScale={1,1}`, and `preExposure=0` or `1`. Use the native SDK's documented color/depth conventions and compose UI after reconstruction. Set `currentResolution` to the fixed render dimensions. Set `RESET_HISTORY` for the first frame, camera cuts, resumed rendering or other discontinuities.

Record `CmdDispatchUpscale` into an open primary graphics/compute command buffer outside rendering. The adapter records directly into the native Vulkan command buffer with stack descriptors. The application owns transitions, submission, inter-queue synchronization, overlap limits and resource reuse. Rebind graphics/compute state used by subsequent work; ANF can change native bindings and invalidate application tile-memory contents. Dispatch errors are reported through OpenRHI's callback because `CmdDispatchUpscale` returns void.

Wait for all referencing submissions before destroying the upscaler, images, descriptors, command objects or device. SDK techniques are destroyed before their instance and runtime library.

## Verification and device status

Record source and SDK hashes, build coverage, packaging checks and native execution separately for each qualification run.

`OpenRHI.UpscalerAnfContract` exercises the production adapter with a deterministic SDK function table, covering requirements, mappings, creation/failure cleanup, reset/jitter/in-flight data and allocation-free adapter dispatch. `OpenRHI.SDKPackaging` checks staged runtimes and licenses. CI builds Android Debug/Release with ANF enabled and disabled, and runs the host contract tests separately.

The [Native Lab APK](../Tests/AndroidDeviceLab/README.md) attempts real technique creation, then dispatches changing-color frames and verifies readback on compatible hardware. Use the actual technique-creation result for fallback. Qualify native dispatch, temporal image quality and sustained performance on each supported device.

Frame generation has a separate presentation/synchronization contract and remains a separate integration path. The implemented adapter follows the SDK's [SR rendering and recorded-dispatch contract](https://github.com/SnapdragonGameStudios/adreno-neural-fusion/blob/a144fbab47ad586adf64c5a1574be73064550f1e/INTEGRATION-GUIDE.md).
