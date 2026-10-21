@echo off
setlocal
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
python "%ROOT%\Scripts\Build\driver.py" build --all-configurations %*
exit /b %ERRORLEVEL%
