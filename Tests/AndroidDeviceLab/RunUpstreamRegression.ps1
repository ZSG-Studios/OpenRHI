param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [string]$BuildDirectory,
    [string]$BinaryDirectory,
    [string]$DeviceSerial,
    [string]$Adb = 'adb'
)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (!$BuildDirectory) { $BuildDirectory = Join-Path $repo "_Build/upstream-android/$Configuration" }
if (!$BinaryDirectory) { $BinaryDirectory = Join-Path $repo "_Bin/upstream-android/$Configuration" }
$BuildDirectory = (Resolve-Path -LiteralPath $BuildDirectory).Path
$BinaryDirectory = (Resolve-Path -LiteralPath $BinaryDirectory).Path
$output = Join-Path $repo ("_Tmp/upstream-device/$Configuration/" + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$remote = "/data/local/tmp/openrhi-upstream-regression/$Configuration"
$deviceArguments = @()
if ($DeviceSerial) { $deviceArguments = @('-s', $DeviceSerial) }
New-Item -ItemType Directory -Force -Path $output | Out-Null

function Run-Adb([string[]]$Arguments, [string]$LogName) {
    & $Adb @deviceArguments @Arguments *> (Join-Path $output "$LogName.log")
    if ($LASTEXITCODE -ne 0) { throw "adb $LogName failed with exit $LASTEXITCODE; see $output" }
}

$programs = @('OpenRHIPublicAPI', 'OpenRHITextureViewContract', 'OpenRHIDevicePolicy',
    'OpenRHISmoke', 'OpenRHIUpstreamRegression', 'OpenRHIQualification')
$shaders = @('DescriptorCompute.spirv', 'IndirectDraw.vs.spirv', 'IndirectDraw.ps.spirv')
$cap = Join-Path $BuildDirectory 'Tests/VulkanInstanceCap/libvulkan.so'
foreach ($path in (@($cap, (Join-Path $BinaryDirectory 'libOpenRHI.so')) +
        @($programs | ForEach-Object { Join-Path $BinaryDirectory $_ }) +
        @($shaders | ForEach-Object { Join-Path $BuildDirectory "Tests/$_" }))) {
    if (!(Test-Path -LiteralPath $path)) { throw "Missing test input: $path" }
}

Run-Adb @('shell', 'mkdir', '-p', "$remote/capped") 'mkdir'
foreach ($file in (@('libOpenRHI.so') + $programs)) {
    Run-Adb @('push', (Join-Path $BinaryDirectory $file), "$remote/$file") "push-$file"
}
if (Test-Path -LiteralPath (Join-Path $BinaryDirectory 'libanf.so')) {
    foreach ($license in @('LICENSE.txt', 'LICENSE-BSD-3-Clause.txt')) {
        $licensePath = Join-Path $BinaryDirectory "ThirdPartyLicenses/anf/$license"
        if (!(Test-Path -LiteralPath $licensePath)) { throw "Missing ANF deployment license: $licensePath" }
    }
    Run-Adb @('push', (Join-Path $BinaryDirectory 'libanf.so'), "$remote/libanf.so") 'push-anf'
}
if (Test-Path -LiteralPath (Join-Path $BinaryDirectory 'ThirdPartyLicenses')) {
    Run-Adb @('push', (Join-Path $BinaryDirectory 'ThirdPartyLicenses'), "$remote/") 'push-licenses'
}
Run-Adb @('push', $cap, "$remote/capped/libvulkan.so") 'push-instance-cap'
foreach ($program in $programs) {
    Run-Adb @('shell', 'chmod', '755', "$remote/$program") "chmod-$program"
}
foreach ($shader in $shaders) {
    Run-Adb @('push', (Join-Path $BuildDirectory "Tests/$shader"), "$remote/$shader") "push-$shader"
}

foreach ($command in @('OpenRHIPublicAPI', 'OpenRHITextureViewContract', 'OpenRHIDevicePolicy',
        'OpenRHISmoke VK', 'OpenRHIUpstreamRegression VK')) {
    Run-Adb @('shell', "LD_LIBRARY_PATH=$remote $remote/$command") $command.Replace(' ', '-')
}
Run-Adb @('shell', "LD_PRELOAD=$remote/capped/libvulkan.so LD_LIBRARY_PATH=$remote $remote/OpenRHIUpstreamRegression VK --instance-cap") 'instance-cap'
Run-Adb @('shell', "LD_LIBRARY_PATH=$remote $remote/OpenRHIQualification --api VK --iterations 8 --native-validation --shader $remote/DescriptorCompute.spirv --vs $remote/IndirectDraw.vs.spirv --ps $remote/IndirectDraw.ps.spirv") 'resources'
Run-Adb @('shell', 'getprop', 'ro.product.model') 'device-model'
Run-Adb @('shell', 'getprop', 'ro.build.version.sdk') 'android-api'
Write-Output "Android $Configuration upstream regression: PASS ($output)"
