@echo off
rem  env_mingw64.bat  – run any command with MinGW-w64 runtime on PATH
setlocal
set "MINGW_BIN=C:\msys64\mingw64\bin"

if not exist "%MINGW_BIN%\libstdc++-6.dll" (
    echo FATAL: %MINGW_BIN% not found. 1>&2
    exit /b 1
)

set "PATH=%MINGW_BIN%;%PATH%"
rem ---- forward every argument unchanged ----
%*
endlocal
