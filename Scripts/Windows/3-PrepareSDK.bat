@echo off
setlocal
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"

set "SELF=%ROOT%"
set "SDK=%ROOT%\_OpenRHI_SDK"

echo %SDK%: ROOT=%ROOT%, SELF=%SELF%

if exist "%SDK%" powershell -NoProfile -File "%~dp0Recycle.ps1" -Names _OpenRHI_SDK
if errorlevel 1 exit /B 1

mkdir "%SDK%\Include\Extensions"
mkdir "%SDK%\CMake"
mkdir "%SDK%\Docs"
copy "%SELF%\Docs\*.md" "%SDK%\Docs"
copy "%SELF%\CMake\*.cmake" "%SDK%\CMake"
mkdir "%SDK%\Lib\Debug"
mkdir "%SDK%\Lib\Release"

copy "%SELF%\Include\*" "%SDK%\Include"
copy "%SELF%\Include\Extensions\*" "%SDK%\Include\Extensions"
copy "%SELF%\LICENSE.txt" "%SDK%"
copy "%SELF%\THIRD_PARTY_NOTICES.md" "%SDK%"
copy "%SELF%\README.md" "%SDK%"
copy "%SELF%\openrhi.natvis" "%SDK%"

copy "%ROOT%\_Bin\Debug\OpenRHI.dll" "%SDK%\Lib\Debug"
copy "%ROOT%\_Bin\Debug\OpenRHI.lib" "%SDK%\Lib\Debug"
copy "%ROOT%\_Bin\Debug\OpenRHI.pdb" "%SDK%\Lib\Debug"
copy "%ROOT%\_Bin\Release\OpenRHI.dll" "%SDK%\Lib\Release"
copy "%ROOT%\_Bin\Release\OpenRHI.lib" "%SDK%\Lib\Release"

for %%C in (Debug Release) do (
    if exist "%ROOT%\_Bin\%%C\ThirdPartyLicenses" xcopy /E /I /Y "%ROOT%\_Bin\%%C\ThirdPartyLicenses" "%SDK%\Lib\%%C\ThirdPartyLicenses" >nul
    if exist "%ROOT%\_Bin\%%C\AgilitySDK" xcopy /E /I /Y "%ROOT%\_Bin\%%C\AgilitySDK" "%SDK%\Lib\%%C\AgilitySDK" >nul
    for %%D in (amd_ags_x64.dll amd_fidelityfx_loader_dx12.dll amd_fidelityfx_upscaler_dx12.dll amd_fidelityfx_denoiser_dx12.dll amd_fidelityfx_radiancecache_dx12.dll amd_fidelityfx_vk.dll NRC_D3D12.dll NRC_Vulkan.dll cudart64_13.dll nvrtc64_130_0.dll nvrtc-builtins64_134.dll nvngx_dlss.dll nvngx_dlssd.dll libxess.dll) do (
        if exist "%ROOT%\_Bin\%%C\%%D" (
            copy /Y "%ROOT%\_Bin\%%C\%%D" "%SDK%\Lib\%%C" >nul
            if errorlevel 1 exit /B 1
        )
    )
)
exit /B %ERRORLEVEL%
