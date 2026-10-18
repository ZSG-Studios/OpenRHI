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
copy "%SELF%\CMake\*.cmake" "%SDK%\CMake"
copy "%SELF%\SHADERS.md" "%SDK%"
mkdir "%SDK%\Lib\Debug"
mkdir "%SDK%\Lib\Release"

copy "%SELF%\Include\*" "%SDK%\Include"
copy "%SELF%\Include\Extensions\*" "%SDK%\Include\Extensions"
copy "%SELF%\LICENSE.txt" "%SDK%"
copy "%SELF%\THIRD_PARTY_NOTICES.md" "%SDK%"
copy "%SELF%\README.md" "%SDK%"
copy "%SELF%\MIGRATION.md" "%SDK%"
copy "%SELF%\QUALIFICATION.md" "%SDK%"
copy "%SELF%\openrhi.natvis" "%SDK%"

copy "%ROOT%\_Bin\Debug\OpenRHI.dll" "%SDK%\Lib\Debug"
copy "%ROOT%\_Bin\Debug\OpenRHI.lib" "%SDK%\Lib\Debug"
copy "%ROOT%\_Bin\Debug\OpenRHI.pdb" "%SDK%\Lib\Debug"
copy "%ROOT%\_Bin\Release\OpenRHI.dll" "%SDK%\Lib\Release"
copy "%ROOT%\_Bin\Release\OpenRHI.lib" "%SDK%\Lib\Release"

for %%C in (Debug Release) do (
    if exist "%ROOT%\_Bin\%%C\ThirdPartyLicenses" xcopy /E /I /Y "%ROOT%\_Bin\%%C\ThirdPartyLicenses" "%SDK%\Lib\%%C\ThirdPartyLicenses" >nul
    if exist "%ROOT%\_Bin\%%C\AgilitySDK" xcopy /E /I /Y "%ROOT%\_Bin\%%C\AgilitySDK" "%SDK%\Lib\%%C\AgilitySDK" >nul
    if exist "%ROOT%\_Bin\%%C\amd_ags_x64.dll" copy "%ROOT%\_Bin\%%C\amd_ags_x64.dll" "%SDK%\Lib\%%C" >nul
)
exit /B %ERRORLEVEL%
