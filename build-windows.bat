@echo off
setlocal

if "%~1"=="" (
    set "CONFIG=Debug"
) else (
    set "CONFIG=%~1"
)

if /I "%CONFIG%"=="Debug" (
    set "PRESET=windows-msvc-debug"
    set "BUILD_PRESET=windows-msvc-debug"
) else if /I "%CONFIG%"=="Release" (
    set "PRESET=windows-msvc-release"
    set "BUILD_PRESET=windows-msvc-release"
) else (
    echo ERROR: Unknown configuration "%CONFIG%"
    echo.
    echo Usage:
    echo     build-windows.bat Debug
    echo     build-windows.bat Release
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Finding MSVC Build Tools
echo ========================================
echo.

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe was not found.
    echo.
    pause
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VSINSTALL=%%i"
)

if not defined VSINSTALL (
    echo ERROR: MSVC Build Tools were not found.
    echo.
    pause
    exit /b 1
)

echo MSVC installation:
echo %VSINSTALL%
echo.

echo ========================================
echo Initializing MSVC x64 environment
echo ========================================
echo.

call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"

if errorlevel 1 (
    echo ERROR: Failed to initialize MSVC.
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Compiler
echo ========================================
echo.

where cl

if errorlevel 1 (
    echo ERROR: cl.exe was not found.
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo CMake
echo ========================================
echo.

where cmake
cmake --version

echo.
echo ========================================
echo Ninja
echo ========================================
echo.

where ninja
ninja --version

echo.
echo ========================================
echo Configuration: %CONFIG%
echo Preset: %PRESET%
echo ========================================
echo.

cmake --preset "%PRESET%"

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed.
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Building
echo ========================================
echo.

cmake --build --preset "%BUILD_PRESET%"

if errorlevel 1 (
    echo.
    echo ERROR: Build failed.
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully
echo ========================================
echo.

pause