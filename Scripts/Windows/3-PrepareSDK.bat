@echo off
setlocal
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
python "%ROOT%\Scripts\Build\driver.py" sdk %*
exit /b %ERRORLEVEL%
