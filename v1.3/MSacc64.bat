@echo off
setlocal
title Windows 11 x64 ACE / Jet Database Redistributable Installer

:: ============================================================================
:: SAFE ANSI COLOR INITIALIZATION (Single-line, no nested parens)
:: ============================================================================
for /f %%A in ('echo prompt $E^| cmd') do set "ESC=%%A"
set "CYAN=%ESC%[96m"
set "GREEN=%ESC%[92m"
set "YELLOW=%ESC%[93m"
set "RED=%ESC%[91m"
set "GRAY=%ESC%[90m"
set "WHITE=%ESC%[97m"
set "RESET=%ESC%[0m"

echo %CYAN%======================================================================%RESET%
echo %WHITE%  WINDOWS 11 x64 ACE / JET REDISTRIBUTABLE SETUP  VERIFICATION%RESET%
echo %CYAN%======================================================================%RESET%

:: ============================================================================
:: [1/3] CHECK IF x64 ACE ODBC DRIVER IS ALREADY REGISTERED
:: ============================================================================
echo.
echo %YELLOW%[1/3] Checking 64-bit ODBC Driver registry for Microsoft Access Driver...%RESET%
reg query "HKLM\SOFTWARE\ODBC\ODBCINST.INI\ODBC Drivers" /v "Microsoft Access Driver (*.mdb, *.accdb)" >nul 2>&1
if not errorlevel 1 goto :DRIVER_PRESENT

echo %GRAY%      Not found. Installing 64-bit ACE Redistributable via winget...%RESET%

:: ============================================================================
:: [2/3] INSTALL VIA WINGET
:: ============================================================================
echo.
echo %YELLOW%[2/3] Running winget install for Microsoft Access Database Engine (x64)...%RESET%
where winget >nul 2>&1
if errorlevel 1 goto :NO_WINGET

winget install --id Microsoft.AccessDatabaseEngine.2016 --architecture x64 --override "/quiet" --accept-package-agreements --accept-source-agreements
if not errorlevel 1 goto :VERIFY_INSTALL

echo %GRAY%      Trying Microsoft 365 Access Runtime x64 fallback...%RESET%
winget install --id Microsoft.Office.AccessRuntime --architecture x64 --accept-package-agreements --accept-source-agreements
goto :VERIFY_INSTALL

:DRIVER_PRESENT
echo %GREEN%  [OK] 64-bit Microsoft Access Driver (*.mdb, *.accdb) is already installed.%RESET%

:: ============================================================================
:: [3/3] VERIFY ODBC DRIVER, ACE DLL  DAO.DBENGINE.120 COM REGISTRATION
:: ============================================================================
:VERIFY_INSTALL
echo.
echo %YELLOW%[3/3] Verifying 64-bit ODBC registry, ACE DLL, and COM registration...%RESET%

set "TEST_PASS=1"

reg query "HKLM\SOFTWARE\ODBC\ODBCINST.INI\ODBC Drivers" /v "Microsoft Access Driver (*.mdb, *.accdb)" >nul 2>&1
if errorlevel 1 goto :FAIL_ODBC
echo %GREEN%  [OK] ODBC Driver registered: Microsoft Access Driver (*.mdb, *.accdb)%RESET%
goto :CHECK_COM

:FAIL_ODBC
echo %RED%  [FAIL] ODBC Driver entry missing in HKLM\SOFTWARE\ODBC\ODBCINST.INI\ODBC Drivers%RESET%
set "TEST_PASS=0"

:CHECK_COM
reg query "HKLM\SOFTWARE\Classes\DAO.DBEngine.120" >nul 2>&1
if errorlevel 1 goto :FAIL_COM
echo %GREEN%  [OK] COM Class registered: DAO.DBEngine.120 (x64)%RESET%
goto :CHECK_DLL

:FAIL_COM
echo %RED%  [FAIL] COM Class DAO.DBEngine.120 not registered in 64-bit registry.%RESET%
set "TEST_PASS=0"

:CHECK_DLL
set "ACE_DLL="
reg query "HKLM\SOFTWARE\ODBC\ODBCINST.INI\Microsoft Access Driver (*.mdb, *.accdb)" /v Driver > "%TEMP%\ace_reg.txt" 2>nul
for /f "tokens=2*" %%A in ('find /I "Driver" "%TEMP%\ace_reg.txt"') do set "ACE_DLL=%%B"
if exist "%TEMP%\ace_reg.txt" del /f /q "%TEMP%\ace_reg.txt" >nul 2>&1

if not defined ACE_DLL goto :FAIL_DLL
echo %GRAY%       DLL Path: %ACE_DLL%%RESET%
if not exist "%ACE_DLL%" goto :FAIL_DLL
echo %GREEN%  [OK] Driver binary verified on disk.%RESET%
goto :SUMMARY

:FAIL_DLL
echo %RED%  [FAIL] Driver binary missing on disk.%RESET%
set "TEST_PASS=0"

:SUMMARY
echo.
echo %CYAN%======================================================================%RESET%
if "%TEST_PASS%"=="1" goto :PASS_EXIT

echo %RED%  STATUS: FAILED - RUN THIS SCRIPT AS ADMINISTRATOR AND RETRY.%RESET%
echo %CYAN%======================================================================%RESET%
pause
exit /b 1

:PASS_EXIT
echo %GREEN%  STATUS: PASSED - WIN11 x64 ACE / JET RUNTIME READY.%RESET%
echo %CYAN%======================================================================%RESET%
pause
exit /b 0

:NO_WINGET
echo %RED%[ERROR] winget is not available in PATH. Install App Installer from Microsoft Store.%RESET%
pause
exit /b 1
