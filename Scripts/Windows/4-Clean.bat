@echo off
powershell -NoProfile -File "%~dp0Recycle.ps1" -Names Build,Bin,.xmake,_Bin,_Build,_Shaders,_Tmp,_OpenRHI_SDK
exit /B %ERRORLEVEL%
