@echo off
setlocal
cd /d "%~dp0"

echo [Process] Terminating any lingering MSVC compiler or linker instances...
taskkill /f /im cl.exe >nul 2>&1
taskkill /f /im link.exe >nul 2>&1
taskkill /f /im mspdbsrv.exe >nul 2>&1
taskkill /f /im rc.exe >nul 2>&1

echo [Process] Terminating any instances using the target binaries...
taskkill /f /fi "MODULES eq dc2sql.dll" >nul 2>&1
taskkill /f /fi "MODULES eq dc2jet.dll" >nul 2>&1
taskkill /f /fi "MODULES eq dc2sql_cli.exe" >nul 2>&1

set "MSVC_ENV="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" (
    set "MSVC_ENV=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" (
    set "MSVC_ENV=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
)

if not defined MSVC_ENV (
    echo [CRITICAL] MSVC Native compiler toolset batch not detected.
    pause
    exit /b 1
)

call "%MSVC_ENV%" x86

echo =======================================================
echo Compiling Resource Script (dc2sql.res)
echo =======================================================

if not exist "resources" mkdir resources
rc.exe /fo dc2sql.res dc2sql.rc
if %ERRORLEVEL% NEQ 0 (
    echo [Failure] Resource compilation failed.
    pause
    exit /b %ERRORLEVEL%
)
echo [Success] Resource file compiled cleanly.

echo =======================================================
echo Compiling DAO-Backed Backend  SQL Router (dc2sql.dll)
echo =======================================================

cl.exe /LD /O2 /W3 /D "_CRT_SECURE_NO_WARNINGS" /D "BUILDING_JET_DLL" ^
    /I"include" ^
    /I"DX-TRACK" ^
    source\dc2jet.c ^
    source\dc2sql.c ^
    source\dc2sql_diag.c ^
    DX-TRACK\Tracker.c ^
    dc2sql.res ^
    /link /DEF:dc2sql.def odbc32.lib odbccp32.lib ole32.lib oleaut32.lib user32.lib /OUT:dc2sql.dll
if %ERRORLEVEL% NEQ 0 (
    echo [Failure] dc2sql.dll build failed.
    pause
    exit /b %ERRORLEVEL%
)



echo [Success] dc2jet.dll compiled cleanly with resources.

echo =======================================================
echo Compiling CLI Runner Executable (dc2sql_cli.exe)
echo =======================================================

cl.exe /O2 /W3 /D "_CRT_SECURE_NO_WARNINGS" ^
    /I"include" ^
    /I"DX-TRACK" ^
    source\dc2sql_cli.c ^
    dc2sql.res ^
    /link user32.lib /OUT:dc2sql.exe

if %ERRORLEVEL% NEQ 0 (
    echo [Failure] CLI compilation failed.
    pause
    exit /b %ERRORLEVEL%
)

echo [Success] dc2sql_cli.exe compiled cleanly with resources.
echo =======================================================
echo [All Systems Operational] Ecosystem compiled successfully!
echo =======================================================

endlocal
pause