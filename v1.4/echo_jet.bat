@echo off
setlocal EnableDelayedExpansion
color 0B
cls
cd /d "%~dp0"

set "BIN_DIR=bin"
set "DC2SQL_EXE=bin\dc2sql.exe"
if not exist "%DC2SQL_EXE%" if exist "..\bin\dc2sql.exe" (
    set "BIN_DIR=..\bin"
    set "DC2SQL_EXE=..\bin\dc2sql.exe"
)
if not exist "%DC2SQL_EXE%" if exist "dc2sql.exe" (
    set "BIN_DIR=."
    set "DC2SQL_EXE=dc2sql.exe"
)

if not exist "%DC2SQL_EXE%" (
    echo [ERROR] Could not locate dc2sql.exe in bin\ or relative directories.
    endlocal
    pause
    exit /b 1
)

set "PATH=%~dp0%BIN_DIR%;%PATH%"

echo ======================================================================
echo    CYBORG MATRIX FILESYSTEM  TABLE LIFECYCLE AUTOMATION V1.5 (ACCDB)
echo    OPERATOR: ROBO-ROOK // SYSTEM: JOE-TRON-DT (WIN11 ACE)
echo    BINARY TARGET: [%DC2SQL_EXE%]
echo ======================================================================
echo.

set "DB_NAME=eclipse_store.accdb"
set "LEGACY_MDB=eclipse_store.mdb"
set "TEMP_DIR=temp_vault"

:: --- SANITIZE ENVIRONMENT ---
if exist "%DB_NAME%" del /f /q "%DB_NAME%" >nul 2>&1
if exist "%LEGACY_MDB%" del /f /q "%LEGACY_MDB%" >nul 2>&1
if exist "%TEMP_DIR%\%DB_NAME%" del /f /q "%TEMP_DIR%\%DB_NAME%" >nul 2>&1
if exist "%TEMP_DIR%\%LEGACY_MDB%" del /f /q "%TEMP_DIR%\%LEGACY_MDB%" >nul 2>&1
if exist "%TEMP_DIR%" rmdir /s /q "%TEMP_DIR%" >nul 2>&1

echo [INIT] Initializing sandbox vault directory: [%TEMP_DIR%]
mkdir "%TEMP_DIR%"
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 1] PROVISIONING ACCDB CONTAINER TABLE (WIN11 ACE)
echo ----------------------------------------------------------------------
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "CREATE TABLE CyborgNodes (NodeID COUNTER PRIMARY KEY, OperatorCode TEXT(64), Subsystem TEXT(64), StatusLog MEMO)" -diag
if errorlevel 1 goto :fail
echo.
echo.
echo.
echo [INIT] Seeding cyber-military telemetry records into CyborgNodes...
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "INSERT INTO CyborgNodes (OperatorCode, Subsystem, StatusLog) VALUES ('ROBO-ROOK-01', 'DIRECTX-12-PIPELINE', 'Active render loop initialized')" -diag
if errorlevel 1 goto :fail
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "INSERT INTO CyborgNodes (OperatorCode, Subsystem, StatusLog) VALUES ('CVBGOD-X', 'WIN32-COM-GATEWAY', 'Workspace security token bound')" -diag
if errorlevel 1 goto :fail
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "INSERT INTO CyborgNodes (OperatorCode, Subsystem, StatusLog) VALUES ('DEMONIZER', 'ACE-ODBC-ENGINE', 'Windows 11 ACCDB driver engaged')" -diag
if errorlevel 1 goto :fail
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 2] DISPLAYING PRIMARY TABLE STATE ('CyborgNodes')
echo ----------------------------------------------------------------------
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem, StatusLog FROM CyborgNodes" -diag
if errorlevel 1 goto :fail
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 3] VERIFYING EXPANDED -LANG ^& -ACK SERIALIZATION SWITCHES
echo ----------------------------------------------------------------------

echo [AVIS] --- [3.1] Python Dictionary List Output with MS-Jet Acknowledgment (-lang python -ack) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang python -ack
echo.
echo.

echo [AVIS] --- [3.2] JavaScript Object Output with MS-Jet Acknowledgment (-lang js -ack) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang js -ack
echo.
echo.

echo [AVIS] --- [3.3] Pure JSON Array with MS-Jet Acknowledgment (-lang json -ack) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang json -ack
echo.
echo.

echo [AVIS] --- [3.4] C Struct Initializer Output (-lang c) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang c
echo.
echo.

echo [AVIS] --- [3.5] C++ Object Record Output (-lang c++) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang c++
echo.
echo.

echo [AVIS] --- [3.6] Java HashMap Record Output (-lang java) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang java
echo.
echo.

echo [AVIS] --- [3.7] PHP Associative Array Output (-lang php) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang php
echo.
echo.

echo [AVIS] --- [3.8] Comma-Separated Values Output (-lang csv) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang csv
echo.
echo.

echo [AVIS] --- [3.9] INI Configuration Output with MS-Jet Acknowledgment (-lang ini -ack) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang ini -ack
echo.
echo.

echo [AVIS] --- [3.10] XML Document Output with MS-Jet Acknowledgment (-lang xml -ack) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang xml -ack
echo.
echo.

echo [AVIS] --- [3.11] VBScript Dictionary Script Output (-lang vbs -ack) ---
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang vbs -ack
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 4] CLONING SCHEMA AND DATA TO TEMPORARY MATRIX
echo ----------------------------------------------------------------------
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "CREATE TABLE CyborgNodes_Temp (NodeID LONG, OperatorCode TEXT(64), Subsystem TEXT(64), StatusLog MEMO)" -diag
if errorlevel 1 goto :fail
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "INSERT INTO CyborgNodes_Temp (NodeID, OperatorCode, Subsystem, StatusLog) SELECT NodeID, OperatorCode, Subsystem, StatusLog FROM CyborgNodes" -diag
if errorlevel 1 goto :fail
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 5] DISPLAYING TEMPORARY TABLE STATE ('CyborgNodes_Temp')
echo ----------------------------------------------------------------------
"%DC2SQL_EXE%" -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem, StatusLog FROM CyborgNodes_Temp" -diag
if errorlevel 1 goto :fail
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 6] FILESYSTEM RELOCATION TO SANDBOX VAULT
echo ----------------------------------------------------------------------
if not exist "%DB_NAME%" (
    echo [ERROR] Expected %DB_NAME% to exist before relocation.
    goto :fail
)
echo [MOVE] Shifting %DB_NAME% into %TEMP_DIR%\...
move /y "%DB_NAME%" "%TEMP_DIR%\%DB_NAME%" >nul
if errorlevel 1 goto :fail
echo [FILESYSTEM] Active Vault Directory Listing:
dir /w /b "%TEMP_DIR%"
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 7] REMOTE QUERYING RELOCATED DATABASE CONTAINER
echo ----------------------------------------------------------------------
"%DC2SQL_EXE%" -db "%TEMP_DIR%\%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem, StatusLog FROM CyborgNodes_Temp" -diag
if errorlevel 1 goto :fail
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 8] SYSTEM TEARDOWN ^& REPOSITORY SANITIZATION
echo ----------------------------------------------------------------------
echo [CLEANUP] Dropping remote database tables...
"%DC2SQL_EXE%" -db "%TEMP_DIR%\%DB_NAME%" -query "DROP TABLE CyborgNodes" -diag
"%DC2SQL_EXE%" -db "%TEMP_DIR%\%DB_NAME%" -query "DROP TABLE CyborgNodes_Temp" -diag
echo [CLEANUP] Purging database container and vault directory...
if exist "%TEMP_DIR%\%DB_NAME%" del /f /q "%TEMP_DIR%\%DB_NAME%" >nul 2>&1
if exist "%TEMP_DIR%" rmdir /s /q "%TEMP_DIR%" >nul 2>&1
echo.
echo.
echo.
echo ======================================================================
echo   STATUS: SUCCESS - ALL MATRIX LOOPS, -LANG ^& -ACK SWITCHES COMPLETED.
echo ======================================================================
endlocal
pause
exit /b 0

:fail
echo.
echo ======================================================================
echo   STATUS: FAILED - CHECK DIAGNOSTICS ABOVE FOR ACCDB / ACE ERRORS.
echo ======================================================================
endlocal
pause
exit /b 1