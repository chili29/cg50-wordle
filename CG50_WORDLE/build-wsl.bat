@echo off
setlocal
where wsl >nul 2>nul
if errorlevel 1 (
  echo WSL is not installed. Install WSL2 and an Ubuntu distribution first.
  pause
  exit /b 1
)
for /f "delims=" %%i in ('wsl wslpath -u "%~dp0"') do set "LINUX_DIR=%%i"
if not defined LINUX_DIR (
  echo Could not translate this folder path into WSL.
  pause
  exit /b 1
)
wsl -e bash -lc "cd '%LINUX_DIR%' && fxsdk build-cg"
if errorlevel 1 (
  echo Build failed. Confirm fxSDK, gint, and the SH toolchain are installed in Ubuntu.
  pause
  exit /b 1
)
echo Build completed. WORDLE.g3a should be in this folder.
pause
