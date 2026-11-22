@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

REM ============================================================================
REM FC Project - Direct UBT Build & Launch Unreal Editor (No Rider/VS overhead)
REM Usage:
REM   BuildAndRun.bat                 (Build FCEditor Development & launch)
REM   BuildAndRun.bat DebugGame       (Build FCEditor DebugGame & launch)
REM   BuildAndRun.bat -clean          (Clean build & launch)
REM   BuildAndRun.bat -nobuild        (Launch editor directly without building)
REM   BuildAndRun.bat -nopause        (Do not pause on error / non-interactive)
REM ============================================================================

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

set "UPROJECT=%PROJECT_ROOT%\FC.uproject"
set "TARGET_NAME=FCEditor"
set "PLATFORM=Win64"
set "CONFIG=Development"
set "CLEAN_FLAG="
set "SKIP_BUILD=0"
set "NO_PAUSE=0"

:parse_args
if "%~1"=="" goto after_args
if /I "%~1"=="-clean" set "CLEAN_FLAG=-Clean"
if /I "%~1"=="/clean" set "CLEAN_FLAG=-Clean"
if /I "%~1"=="-nobuild" set "SKIP_BUILD=1"
if /I "%~1"=="/nobuild" set "SKIP_BUILD=1"
if /I "%~1"=="-nopause" set "NO_PAUSE=1"
if /I "%~1"=="/nopause" set "NO_PAUSE=1"
if /I "%~1"=="Development" set "CONFIG=Development"
if /I "%~1"=="DebugGame" set "CONFIG=DebugGame"
if /I "%~1"=="Debug" set "CONFIG=Debug"
if /I "%~1"=="-help" goto show_help
if /I "%~1"=="--help" goto show_help
if /I "%~1"=="/?" goto show_help
shift
goto parse_args

:after_args
if not exist "%UPROJECT%" (
    for %%F in ("%PROJECT_ROOT%\*.uproject") do (
        set "UPROJECT=%%~fF"
        set "TARGET_NAME=%%~nFEditor"
        goto found_uproject
    )
    echo [ERROR] Could not find .uproject file in "%PROJECT_ROOT%".
    goto fail_exit
)

:found_uproject

REM Locate Unreal Engine 5.8
set "UE_DIR="
for %%E in ("%UE_5_8_PATH%" "%UNREAL_ENGINE_PATH%" "%UE_ROOT%" "%ENGINE_PATH%" "%UE_ENGINE_DIR%") do (
    if not "%%~E"=="" if exist "%%~E\Engine\Build\BatchFiles\Build.bat" (
        set "UE_DIR=%%~E"
        goto found_ue
    )
)

if exist "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" (
    set "UE_DIR=C:\Program Files\Epic Games\UE_5.8"
    goto found_ue
)

for /f "tokens=2*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v "InstalledDirectory" 2^>nul ^| find /i "InstalledDirectory"') do (
    if exist "%%B\Engine\Build\BatchFiles\Build.bat" (
        set "UE_DIR=%%B"
        goto found_ue
    )
)

echo [ERROR] Could not locate Unreal Engine 5.8 (Build.bat not found).
echo Set UE_5_8_PATH or UNREAL_ENGINE_PATH environment variable.
goto fail_exit

:found_ue
set "BUILD_BAT=%UE_DIR%\Engine\Build\BatchFiles\Build.bat"
set "EDITOR_EXE=%UE_DIR%\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%EDITOR_EXE%" (
    echo [ERROR] UnrealEditor.exe not found at "%EDITOR_EXE%".
    goto fail_exit
)

echo ============================================================================
echo  Project : %UPROJECT%
echo  Engine  : %UE_DIR%
echo  Target  : %TARGET_NAME% %PLATFORM% %CONFIG%
echo ============================================================================

if "%SKIP_BUILD%"=="1" goto launch_editor

REM Warn if UnrealEditor.exe is already running (may lock DLLs during link)
tasklist /FI "IMAGENAME eq UnrealEditor.exe" 2>nul | find /I "UnrealEditor.exe" >nul
if %ERRORLEVEL%==0 (
    echo.
    echo [WARNING] UnrealEditor.exe is currently running. Building while the editor is open may fail due to locked DLLs.
    if "%NO_PAUSE%"=="0" (
        choice /C YNC /N /M "[Y] Close UnrealEditor and build   [N] Cancel   [C] Build anyway: "
        if errorlevel 3 goto do_build
        if errorlevel 2 (
            echo Cancelled by user.
            exit /b 0
        )
        if errorlevel 1 (
            echo Closing UnrealEditor.exe...
            taskkill /IM UnrealEditor.exe /F >nul 2>&1
            timeout /t 2 /nobreak >nul
        )
    )
)

:do_build
echo.
echo [1/2] Building %TARGET_NAME% (%PLATFORM% %CONFIG%)...
call "%BUILD_BAT%" %TARGET_NAME% %PLATFORM% %CONFIG% -Project="%UPROJECT%" -WaitMutex -NoHotReload %CLEAN_FLAG%
set "BUILD_EXIT=%ERRORLEVEL%"

if not "%BUILD_EXIT%"=="0" (
    echo.
    echo ============================================================================
    echo [FAILED] Build failed with exit code %BUILD_EXIT%. Editor will NOT launch.
    echo ============================================================================
    goto fail_exit_code
)

:launch_editor
echo.
echo [2/2] Build succeeded! Launching Unreal Editor...
start "" "%EDITOR_EXE%" "%UPROJECT%"
exit /b 0

:show_help
echo Usage: BuildAndRun.bat [Development^|DebugGame^|Debug] [-clean] [-nobuild] [-nopause]
exit /b 0

:fail_exit
set "BUILD_EXIT=1"

:fail_exit_code
if "%NO_PAUSE%"=="0" pause
exit /b %BUILD_EXIT%
