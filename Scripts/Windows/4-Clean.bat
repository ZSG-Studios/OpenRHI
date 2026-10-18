@echo off
powershell -NoProfile -File "%~dp0Recycle.ps1" -Names build,_Bin,_Build,_Shaders,_OpenRHI_SDK
exit /B %ERRORLEVEL%
