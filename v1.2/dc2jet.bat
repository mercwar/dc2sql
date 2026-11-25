@echo off
setlocal EnableDelayedExpansion
title DC2SQL x64 Suite Builder (dc2sql.dll + dc2sql.exe)

:: ============================================================================
:: CONFIGURATION & PATHS
:: ============================================================================
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "BIN=%ROOT%\bin"
set "OBJ=%ROOT%\obj"
set "LIB_DIR=%ROOT%\lib"
set "INC_DIR=%ROOT%\include"
set "SRC_DIR=%ROOT%\source"
set "CRASH_LOG=%ROOT%\build_crash.log"

:: Name of your separate language parser C file inside source\
set "LANG_C=%SRC_DIR%\dc2jet_fmt.c"

set "RC_FILE=%ROOT%\dc2sql.rc"
set "DEF_FILE=%ROOT%\dc2sql.def"
set "RES_OUT=%OBJ%\dc2sql.res"

set "OUT_DLL=%BIN%\dc2sql.dll"
set "OUT_LIB=%LIB_DIR%\dc2sql.lib"
set "OUT_EXE=%BIN%\dc2sql.exe"

:: Initialize ANSI Escape Colors
for /f %%A in ('echo prompt $E ^| cmd') do set "ESC=%%A"
set "RED=%ESC%[91m"
set "GREEN=%ESC%[92m"
set "YELLOW=%ESC%[93m"
set "CYAN=%ESC%[96m"
set "WHITE=%ESC%[97m"
set "GRAY=%ESC%[90m"
set "RESET=%ESC%[0m"

:: ============================================================================
:: [1/5] CLEAN OLD ARTIFACTS & LOCKED PROCESSES
:: ============================================================================
set "STEP=1/5 - Clean Legacy Artifacts"
echo %CYAN%====================================================================%RESET%
echo %WHITE%  DC2SQL x64 SUITE BUILDER  %GRAY%^|%RESET%  %CYAN%dc2sql.dll + dc2sql.exe%RESET%
echo %CYAN%====================================================================%RESET%
echo %YELLOW%[1/5] Stopping active instances and removing old build files...%RESET%

taskkill /f /im cl.exe >nul 2>&1
taskkill /f /im link.exe >nul 2>&1
taskkill /f /im rc.exe >nul 2>&1
taskkill /f /im dc2sql.exe >nul 2>&1
taskkill /f /fi "MODULES eq dc2sql.dll" >nul 2>&1

:: Remove stale legacy binaries in root or output folders
for %%F in (
    "%ROOT%\dc2sql.dll" "%ROOT%\dc2sql.exe" "%ROOT%\dc2jet.dll" "%ROOT%\dc2sql_cli.exe"
    "%ROOT%\dc2sql.res" "%ROOT%\*.obj" "%ROOT%\*.exp" "%ROOT%\*.lib"
    "%OUT_DLL%" "%OUT_EXE%" "%OUT_LIB%" "%RES_OUT%" "%CRASH_LOG%"
) do (
    if exist "%%~F" del /f /q "%%~F" >nul 2>&1
)

if not exist "%BIN%" mkdir "%BIN%"
if not exist "%OBJ%" mkdir "%OBJ%"
if not exist "%LIB_DIR%" mkdir "%LIB_DIR%"
echo %GREEN%      [OK] Workspace cleaned%RESET%

:: ============================================================================
:: [2/5] VERIFY SOURCE, HEADER, DEF & RESOURCE FILES
:: ============================================================================
set "STEP=2/5 - Verify Source Files"
set "DETAIL_LOG="
echo %YELLOW%[2/5] Checking source, definition, and resource files...%RESET%

for %%F in (
    "%SRC_DIR%\dc2jet.c"
    "%SRC_DIR%\dc2sql.c"
    "%SRC_DIR%\dc2sql_diag.c"
    "%SRC_DIR%\dc2sql_cli.c"
    "%LANG_C%"
    "%DEF_FILE%"
    "%RC_FILE%"
) do (
    if not exist "%%~F" (
        set "ERR_MSG=Missing required file: %%~F"
        set "LAST_RC=1"
        goto :FAIL
    )
)
echo %GREEN%      [OK] All source, .def, and .rc files present%RESET%

:: ============================================================================
:: [3/5] INITIALIZE MSVC x64 TOOLCHAIN
:: ============================================================================
set "STEP=3/5 - Initialize MSVC x64 Toolchain"
echo %YELLOW%[3/5] Locating MSVC x64 compiler...%RESET%

where cl.exe >nul 2>&1
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!VSWHERE!" (
        for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
            set "VS_PATH=%%I"
        )
    )
    if defined VS_PATH (
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
            call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
        )
    )
)

where cl.exe >nul 2>&1
if errorlevel 1 (
    set "ERR_MSG=cl.exe not found. Install Visual Studio C++ Build Tools or run from x64 Native Tools CMD."
    set "LAST_RC=1"
    goto :FAIL
)
echo %GREEN%      [OK] MSVC x64 toolchain ready%RESET%

:: ============================================================================
:: [4/5] COMPILE RESOURCE SCRIPT (dc2sql.rc -> obj\dc2sql.res)
:: ============================================================================
set "STEP=4/5 - Compile Resource Script"
set "DETAIL_LOG=%OBJ%\rc_build.log"
echo %YELLOW%[4/5] Compiling dc2sql.rc...%RESET%

rc /nologo /I"%ROOT%" /fo"%RES_OUT%" "%RC_FILE%" >"%DETAIL_LOG%" 2>&1
if errorlevel 1 (
    set "ERR_MSG=rc.exe failed to compile dc2sql.rc."
    set "LAST_RC=!errorlevel!"
    goto :FAIL
)
echo %GREEN%      [OK] Built %RES_OUT%%RESET%

:: ============================================================================
:: [5/5] COMPILE dc2sql.dll & dc2sql.exe
:: ============================================================================
set "STEP=5/5 - Compile dc2sql.dll and dc2sql.exe"
set "DETAIL_LOG=%OBJ%\cl_dll_build.log"
echo %YELLOW%[5/5] Compiling dc2sql.dll...%RESET%

cl /nologo /LD /O2 /W3 /TC /D_CRT_SECURE_NO_WARNINGS /DBUILDING_JET_DLL ^
    /I"%INC_DIR%" /I"%SRC_DIR%" ^
    "%SRC_DIR%\dc2jet.c" ^
    "%SRC_DIR%\dc2sql.c" ^
    "%SRC_DIR%\dc2sql_diag.c" ^
    /Fo:"%OBJ%\\" ^
    /Fe:"%OUT_DLL%" ^
    /link "%RES_OUT%" /DEF:"%DEF_FILE%" /IMPLIB:"%OUT_LIB%" ^
    odbc32.lib odbccp32.lib ole32.lib oleaut32.lib user32.lib advapi32.lib >"%DETAIL_LOG%" 2>&1

if errorlevel 1 (
    set "ERR_MSG=cl.exe failed to compile dc2sql.dll."
    set "LAST_RC=!errorlevel!"
    goto :FAIL
)
echo %GREEN%      [OK] Built %OUT_DLL%%RESET%

set "DETAIL_LOG=%OBJ%\cl_exe_build.log"
echo %YELLOW%      Compiling dc2sql.exe...%RESET%

cl /nologo /O2 /W3 /TC /D_CRT_SECURE_NO_WARNINGS ^
    /I"%INC_DIR%" /I"%SRC_DIR%" ^
    "%SRC_DIR%\dc2sql_cli.c" ^
    "%LANG_C%" ^
    /Fo:"%OBJ%\\" ^
    /Fe:"%OUT_EXE%" ^
    /link "%RES_OUT%" user32.lib >"%DETAIL_LOG%" 2>&1

if errorlevel 1 (
    set "ERR_MSG=cl.exe failed to compile dc2sql.exe."
    set "LAST_RC=!errorlevel!"
    goto :FAIL
)

move /y "%LIB_DIR%\*.exp" "%OBJ%\" >nul 2>&1
echo %GREEN%      [OK] Built %OUT_EXE%%RESET%

:: ============================================================================
:: BUILD SUMMARY
:: ============================================================================
echo %CYAN%====================================================================%RESET%
echo %GREEN%  [ALL SYSTEMS OPERATIONAL] Build completed cleanly!%RESET%
echo   %GRAY%DLL :%RESET% %OUT_DLL%
echo   %GRAY%EXE :%RESET% %OUT_EXE%
echo   %GRAY%LIB :%RESET% %OUT_LIB%
echo   %GRAY%RES :%RESET% %RES_OUT%
echo %CYAN%====================================================================%RESET%
pause
exit /b 0

:: ============================================================================
:: ERROR HANDLER
:: ============================================================================
:FAIL
echo %CYAN%--------------------------------------------------------------------%RESET%
echo %RED%  [BUILD FAILED] Step   : %STEP%%RESET%
echo %RED%  [REASON]       %ERR_MSG% (Exit Code: %LAST_RC%)%RESET%
(
    echo DC2SQL BUILD CRASH REPORT - %DATE% %TIME%
    echo STEP: %STEP%
    echo ERROR: %ERR_MSG%
    echo EXIT CODE: %LAST_RC%
    echo ====================================================================
) > "%CRASH_LOG%"
if defined DETAIL_LOG (
    if exist "%DETAIL_LOG%" (
        echo --- Compiler Output --- >> "%CRASH_LOG%"
        type "%DETAIL_LOG%" >> "%CRASH_LOG%"
        echo %GRAY%--- Compiler Output ---%RESET%
        type "%DETAIL_LOG%"
    )
)
echo %YELLOW%  Crash log saved to: %CRASH_LOG%%RESET%
echo %CYAN%====================================================================%RESET%
pause
exit /b %LAST_RC%
