# OpenRHI Native Lab APK

The ARM64 Android app executes OpenRHI's C/C++ API checks, device policies, Vulkan submission/fences, GPU copies and readback, nonuniform descriptor compute, indirect rendering, NVIDIA Image Scaling readback, task/mesh rendering, and a pipeline-cache file roundtrip followed by cached dispatch. The cache test requests failure on a cache miss when the device supports that contract.

The app displays results and writes `files/results.txt`, `files/native.log`, and `files/pipeline-cache.bin` to its private storage. The packaged Khronos layer runs on the resource, mesh, cache and ANF paths when available. ANF has both a standalone SDK compatibility query and the OpenRHI SR adapter workload. The latter negotiates native device/image requirements, creates the SDK technique through OpenRHI, and on compatible drivers dispatches three changing-color frames, checks output readback and exercises history reset. Driver rejection is displayed as SKIP with its reason; a successful support query alone does not pass the dispatch test. The manifest declares the optional public `libOpenCL.so` dependency required by ANF on the S26, as documented in its SDK integration guide.

## Build

Use CMake 3.30+, Ninja, Python, a JDK, Android SDK platform 37.0/build-tools 37.0.0, NDK 30.0.16248370, and Slang 2026.20. `Build.ps1` accepts SDK/dependency paths and uses the normal build's pinned Slang distribution. Supply unchanged Vulkan-Headers v1.4.365, VulkanMemoryAllocator commit `3aa921224c154a0d2c43912bc88e1c42ce1f7607`, and ANF commit `a144fbab47ad586adf64c5a1574be73064550f1e`. The checked-in ANF compatibility probe is reused with the corresponding public SDK headers. Khronos Android validation binaries 1.4.363.0 are packaged along with dependency licenses. Vendor sources remain unmodified.

```powershell
./Tests/AndroidDeviceLab/Build.ps1 -Configuration Debug -AnfSdkRoot C:/SDKs/anf -VulkanHeaders C:/SDKs/Vulkan-Headers -VmaRoot C:/SDKs/VulkanMemoryAllocator -ValidationLayer C:/SDKs/validation/arm64-v8a/libVkLayer_khronos_validation.so
./Tests/AndroidDeviceLab/Build.ps1 -Configuration Release -AnfSdkRoot C:/SDKs/anf -VulkanHeaders C:/SDKs/Vulkan-Headers -VmaRoot C:/SDKs/VulkanMemoryAllocator -ValidationLayer C:/SDKs/validation/arm64-v8a/libVkLayer_khronos_validation.so
```

Supply the pinned dependencies explicitly after a clean checkout and run the normal deploy step to acquire Slang. Replace the example `C:/SDKs` paths with your SDK locations. Generated APKs, native builds, and the development signing key stay under `_Tmp/android-native-lab`. Both configurations use a debuggable application manifest for the device lab. This development certificate is local to the lab build.

## Run and collect

```powershell
adb -s DEVICE install -r _Tmp/android-native-lab/Release/OpenRHI-NativeLab-Release.apk
adb -s DEVICE shell am force-stop com.openrhi.nativelab
adb -s DEVICE shell am start -W -n com.openrhi.nativelab/.MainActivity
adb -s DEVICE exec-out run-as com.openrhi.nativelab cat files/results.txt
adb -s DEVICE exec-out run-as com.openrhi.nativelab cat files/native.log
```

Tests run on a worker thread when the activity is created. Wait for the completed report before collecting results. For automation, require `results.txt` to name the installed configuration and retain the matching raw log. Preserve the prior report before restarting a run. Source and native workload receipts identify the implementation; short correctness samples do not qualify performance or sustained mobile behavior.
