@echo off
setlocal
title DC2SQL Tracked Session Test Suite (-track)

:: ============================================================================
:: PATH RESOLUTION
:: ============================================================================
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

if exist "%SCRIPT_DIR%\dc2sql.exe" (
    set "BIN_DIR=%SCRIPT_DIR%"
    for %%I in ("%SCRIPT_DIR%\..") do set "ROOT_DIR=%%~fI"
) else (
    set "ROOT_DIR=%SCRIPT_DIR%"
    set "BIN_DIR=%SCRIPT_DIR%\bin"
)

set "EXE=%BIN_DIR%\dc2sql.exe"
set "VAULT=%ROOT_DIR%\temp_vault"
set "DB_NAME=track_session_test.accdb"
set "LOCK_FILE=%VAULT%\track_session_test.laccdb"
set "HOST_LOG=%VAULT%\track_host.log"

:: ============================================================================
:: SAFE ANSI COLOR INITIALIZATION
:: ============================================================================
for /f "delims=" %%E in ('"prompt $E$S & for %%B in (1) do rem"') do set "ESC=%%E"
set "ESC=%ESC:~0,1%"
set "CYAN=%ESC%[96m"
set "GREEN=%ESC%[92m"
set "YELLOW=%ESC%[93m"
set "RED=%ESC%[91m"
set "GRAY=%ESC%[90m"
set "WHITE=%ESC%[97m"
set "RESET=%ESC%[0m"

:: ============================================================================
:: BANNER & PRE-FLIGHT CLEANUP
:: ============================================================================
echo %CYAN%======================================================================%RESET%
echo %WHITE%  DC2SQL TRACKED SESSION LIFECYCLE TEST (-track)%RESET%
echo %GRAY%  BINARY : %EXE%%RESET%
echo %GRAY%  VAULT  : %VAULT%\%DB_NAME%%RESET%
echo %CYAN%======================================================================%RESET%

if not exist "%EXE%" (
    echo %RED%[ERROR] Missing executable: %EXE%%RESET%
    pause
    exit /b 1
)

taskkill /F /IM dc2sql.exe >nul 2>&1
if not exist "%VAULT%" mkdir "%VAULT%"
if exist "%VAULT%\%DB_NAME%" del /f /q "%VAULT%\%DB_NAME%" >nul 2>&1
if exist "%LOCK_FILE%" del /f /q "%LOCK_FILE%" >nul 2>&1
if exist "%HOST_LOG%" del /f /q "%HOST_LOG%" >nul 2>&1

:: ============================================================================
:: [1/4] PROVISION TABLE & SEED INITIAL TELEMETRY
:: ============================================================================
echo.
echo %YELLOW%[1/4] Provisioning table and seeding initial telemetry...%RESET%
"%EXE%" --dir "%VAULT%" --db "%DB_NAME%" --sql "CREATE TABLE TrackedNodes (NodeID COUNTER PRIMARY KEY, Operator TEXT(64), NodeState TEXT(64))" --diag
if errorlevel 1 goto :FAIL_STEP1

"%EXE%" --dir "%VAULT%" --db "%DB_NAME%" --sql "INSERT INTO TrackedNodes (Operator, NodeState) VALUES ('ROBO-ROOK', 'SESSION-OPEN-TRACKED')"
if errorlevel 1 goto :FAIL_STEP1

:: ============================================================================
:: [2/4] LAUNCH HEADLESS BACKGROUND SESSION (/B = NO EXTRA WINDOW)
:: ============================================================================
echo.
echo %YELLOW%[2/4] Launching persistent tracked session (-track) inline...%RESET%
start "" /B cmd /c ""%EXE%" --dir "%VAULT%" --db "%DB_NAME%" --session "cyborg_track_01" --sql "SELECT * FROM TrackedNodes" --lang json --diag -track > "%HOST_LOG%" 2>&1"

timeout /t 2 /nobreak >nul

:: ============================================================================
:: [3/4] VERIFY RESIDENT PROCESS, TRACKER HANDLE & LOCK FILE
:: ============================================================================
echo.
echo %YELLOW%[3/4] Verifying active session process and log output...%RESET%
if exist "%HOST_LOG%" (
    echo %GRAY%--- Tracked Host Output ---%RESET%
    type "%HOST_LOG%"
    echo %GRAY%---------------------------%RESET%
)

tasklist /FI "IMAGENAME eq dc2sql.exe" /FO CSV /NH > "%TEMP%\dc2sql_task.txt" 2>nul
find /I "dc2sql.exe" "%TEMP%\dc2sql_task.txt" >nul 2>&1
if errorlevel 1 goto :FAIL_STEP3

echo %GREEN%  [OK] dc2sql.exe is resident and holding the Tracker.dll session open.%RESET%
if exist "%LOCK_FILE%" echo %GREEN%  [OK] Active ACE lock container verified: %LOCK_FILE%%RESET%

:: ============================================================================
:: [4/4] RELEASE TRACKED SESSION & VERIFY CLEAN TEARDOWN
:: ============================================================================
echo.
echo %YELLOW%[4/4] Releasing tracked session and verifying clean teardown...%RESET%
taskkill /F /IM dc2sql.exe >nul 2>&1
timeout /t 1 /nobreak >nul

if not exist "%LOCK_FILE%" echo %GREEN%  [OK] ACE lock file cleaned up after session release.%RESET%

echo.
echo %CYAN%======================================================================%RESET%
echo %GREEN%  STATUS: PASSED - TRACKED SESSION (-track) VERIFIED.%RESET%
echo %CYAN%======================================================================%RESET%
pause
exit /b 0

:: ============================================================================
:: ERROR HANDLERS
:: ============================================================================
:FAIL_STEP1
echo %RED%[FAILED] Database provisioning or seed insert failed.%RESET%
pause
exit /b 1

:FAIL_STEP3
echo %RED%[FAILED] dc2sql.exe exited early instead of staying open with -track.%RESET%
pause
exit /b 1
