param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [string]$SdkRoot = 'C:/EGPTools/android-sdk',
    [string]$NdkVersion = '30.0.16248370',
    [string]$AnfSdkRoot,
    [string]$VulkanHeaders,
    [string]$VmaRoot,
    [string]$ValidationLayer
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$lab = Join-Path $repo '_Tmp/android-native-lab'
$output = Join-Path $lab $Configuration
$native = Join-Path $output 'native'
$assets = Join-Path $output 'assets'
if (!$AnfSdkRoot) { $AnfSdkRoot = Join-Path $repo '_Tmp/anf-device-probe/sdk' }
if (!$VulkanHeaders) { $VulkanHeaders = Join-Path $lab 'vulkan-headers' }
if (!$VmaRoot) { $VmaRoot = Join-Path $lab 'vma' }
if (!$ValidationLayer) { $ValidationLayer = Join-Path $lab 'layers/android-binaries-1.4.363.0/arm64-v8a/libVkLayer_khronos_validation.so' }
$tools = Join-Path $SdkRoot 'build-tools/37.0.0'
$androidJar = Join-Path $SdkRoot 'platforms/android-37.0/android.jar'
$ndk = Join-Path $SdkRoot "ndk/$NdkVersion"
foreach ($path in @($AnfSdkRoot, $VulkanHeaders, $VmaRoot, $ValidationLayer, $androidJar)) {
    if (!(Test-Path -LiteralPath $path)) { throw "Missing build input: $path" }
}
New-Item -ItemType Directory -Force $output, "$assets/shaders", "$assets/licenses", "$output/classes", "$output/dex" | Out-Null
function Run([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit $LASTEXITCODE" }
}
Run 'cmake' @('-S', $PSScriptRoot, '-B', $native, '-G', 'Ninja', "-DCMAKE_BUILD_TYPE=$Configuration",
    "-DCMAKE_TOOLCHAIN_FILE=$ndk/build/cmake/android.toolchain.cmake", '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-33',
    '-DANDROID_STL=c++_static', "-DANF_SDK_ROOT=$AnfSdkRoot", "-DFETCHCONTENT_SOURCE_DIR_VULKAN_HEADERS=$VulkanHeaders",
    "-DFETCHCONTENT_SOURCE_DIR_VMA=$VmaRoot", "-DOPENRHI_SLANG_ROOT=$repo/_Build/_deps/openrhi_slang-src")
Run 'cmake' @('--build', $native, '--parallel', '8')
Get-ChildItem $native -Filter '*.spirv' | Copy-Item -Destination "$assets/shaders/"
Copy-Item "$repo/LICENSE.txt" "$assets/licenses/OpenRHI.txt"
Copy-Item "$repo/THIRD_PARTY_NOTICES.md" "$assets/licenses/THIRD_PARTY_NOTICES.md"
Copy-Item "$AnfSdkRoot/LICENSE.txt" "$assets/licenses/ANF-binary.txt"
Copy-Item "$AnfSdkRoot/LICENSE-BSD-3-Clause.txt" "$assets/licenses/ANF-headers.txt"
foreach ($dependency in @($VulkanHeaders, $VmaRoot)) {
    Get-ChildItem $dependency -Filter 'LICENSE*' | ForEach-Object {
        Copy-Item $_.FullName (Join-Path "$assets/licenses" ((Split-Path $dependency -Leaf) + '-' + $_.Name))
    }
}
$validationLicense = Join-Path $lab 'VVL-LICENSE.txt'
if (!(Test-Path $validationLicense)) {
    Run 'curl.exe' @('-fsSL', '--max-time', '60', 'https://raw.githubusercontent.com/KhronosGroup/Vulkan-ValidationLayers/vulkan-sdk-1.4.363.0/LICENSE.txt', '-o', $validationLicense)
}
Copy-Item $validationLicense "$assets/licenses/Vulkan-ValidationLayers.txt"
Run (Join-Path $tools 'aapt2.exe') @('link', '-o', "$output/base.apk", '-I', $androidJar, '--manifest', "$PSScriptRoot/AndroidManifest.xml", '-A', $assets)
Run 'javac' @('--release', '8', '-classpath', $androidJar, '-d', "$output/classes", "$PSScriptRoot/MainActivity.java")
$classFiles = @(Get-ChildItem "$output/classes" -Recurse -Filter '*.class' | ForEach-Object { $_.FullName })
Run (Join-Path $tools 'd8.bat') (@('--min-api', '33', '--lib', $androidJar, '--output', "$output/dex") + $classFiles)
Copy-Item "$output/base.apk" "$output/unsigned.apk"
Run 'python' @("$PSScriptRoot/Package.py", "$output/unsigned.apk", "$output/dex/classes.dex", "$native/libOpenRHINativeLab.so", "$AnfSdkRoot/lib/arm64-v8a/libanf.so", $ValidationLayer)
Run (Join-Path $tools 'zipalign.exe') @('-f', '-P', '16', '4', "$output/unsigned.apk", "$output/aligned.apk")
$key = Join-Path $lab 'lab-debug.keystore'
if (!(Test-Path $key)) {
    Run 'keytool' @('-genkeypair', '-keystore', $key, '-storepass', 'android', '-keypass', 'android', '-alias', 'androiddebugkey',
        '-keyalg', 'RSA', '-keysize', '2048', '-validity', '10000', '-dname', 'CN=OpenRHI Native Lab', '-noprompt')
}
$apk = Join-Path $output "OpenRHI-NativeLab-$Configuration.apk"
Run (Join-Path $tools 'apksigner.bat') @('sign', '--ks', $key, '--ks-pass', 'pass:android', '--key-pass', 'pass:android', '--out', $apk, "$output/aligned.apk")
Run (Join-Path $tools 'apksigner.bat') @('verify', '--verbose', $apk)
Get-FileHash $apk -Algorithm SHA256
Write-Output "APK: $apk"
