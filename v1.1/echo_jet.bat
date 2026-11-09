@echo off
setlocal
color 0B
cls
cd /d "%~dp0"
echo ======================================================================
echo    CYBORG MATRIX FILESYSTEM  TABLE LIFECYCLE AUTOMATION V1.3
echo    OPERATOR: ROBO-ROOK // SYSTEM: JOE-TRON-DT
echo ======================================================================
echo.

set "DB_NAME=eclipse_store.mdb"
set "TEMP_DIR=temp_vault"

:: --- SANITIZE ENVIRONMENT ---
if exist "%DB_NAME%" del /f /q "%DB_NAME%" >nul 2>&1
if exist "%TEMP_DIR%\%DB_NAME%" del /f /q "%TEMP_DIR%\%DB_NAME%" >nul 2>&1
if exist "%TEMP_DIR%" rmdir "%TEMP_DIR%" >nul 2>&1

echo [INIT] Initializing sandbox vault directory: [%TEMP_DIR%]
mkdir "%TEMP_DIR%"
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 1] PROVISIONING EXCLUSIVE JET 4.0 CONTAINER TABLE
echo ----------------------------------------------------------------------
dc2sql_cli.exe -db "%DB_NAME%" -query "CREATE TABLE CyborgNodes (NodeID COUNTER PRIMARY KEY, OperatorCode TEXT(64), Subsystem TEXT(64), StatusLog MEMO)" -diag
echo.
echo.
echo.
echo [INIT] Seeding cyber-military telemetry records into CyborgNodes...
dc2sql_cli.exe -db "%DB_NAME%" -query "INSERT INTO CyborgNodes (OperatorCode, Subsystem, StatusLog) VALUES ('ROBO-ROOK-01', 'DIRECTX-12-PIPELINE', 'Active render loop initialized')" -diag
dc2sql_cli.exe -db "%DB_NAME%" -query "INSERT INTO CyborgNodes (OperatorCode, Subsystem, StatusLog) VALUES ('CVBGOD-X', 'WIN32-COM-GATEWAY', 'Workspace security token bound')" -diag
dc2sql_cli.exe -db "%DB_NAME%" -query "INSERT INTO CyborgNodes (OperatorCode, Subsystem, StatusLog) VALUES ('DEMONIZER', 'JET-ODBC-ENGINE', 'Exclusive 32-bit driver engaged')" -diag
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 2] DISPLAYING PRIMARY TABLE STATE ('CyborgNodes')
echo ----------------------------------------------------------------------
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem, StatusLog FROM CyborgNodes" -diag
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 3] VERIFYING EXPANDED -LANG  -ACK SERIALIZATION SWITCHES
echo ----------------------------------------------------------------------

echo [AVIS] --- [3.1] Python Dictionary List Output with MS-Jet Acknowledgment (-lang python -ack) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang python -ack
echo.
echo.

echo [AVIS] --- [3.2] JavaScript Object Output with MS-Jet Acknowledgment (-lang js -ack) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang js -ack
echo.
echo.

echo [AVIS] --- [3.3] Pure JSON Array with MS-Jet Acknowledgment (-lang json -ack) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang json -ack
echo.
echo.

echo [AVIS] --- [3.4] C Struct Initializer Output (-lang c) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang c
echo.
echo.

echo [AVIS] --- [3.5] C++ Object Record Output (-lang c++) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang c++
echo.
echo.

echo [AVIS] --- [3.6] Java HashMap Record Output (-lang java) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang java
echo.
echo.

echo [AVIS] --- [3.7] PHP Associative Array Output (-lang php) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang php
echo.
echo.

echo[AVIS]  --- [3.8] Comma-Separated Values Output (-lang csv) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang csv
echo.
echo.

echo [AVIS] --- [3.9] INI Configuration Output with MS-Jet Acknowledgment (-lang ini -ack) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang ini -ack
echo.
echo.

echo [AVIS] --- [3.10] XML Document Output with MS-Jet Acknowledgment (-lang xml -ack) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang xml -ack
echo.
echo.

echo [AVIS] --- [3.11] VBScript Dictionary Script Output (-lang vbs -ack) ---
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem FROM CyborgNodes" -lang vbs -ack
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 4] CLONING SCHEMA AND DATA TO TEMPORARY MATRIX
echo ----------------------------------------------------------------------
dc2sql_cli.exe -db "%DB_NAME%" -query "CREATE TABLE CyborgNodes_Temp (NodeID LONG, OperatorCode TEXT(64), Subsystem TEXT(64), StatusLog MEMO)" -diag
dc2sql_cli.exe -db "%DB_NAME%" -query "INSERT INTO CyborgNodes_Temp (NodeID, OperatorCode, Subsystem, StatusLog) SELECT NodeID, OperatorCode, Subsystem, StatusLog FROM CyborgNodes" -diag
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 5] DISPLAYING TEMPORARY TABLE STATE ('CyborgNodes_Temp')
echo ----------------------------------------------------------------------
dc2sql_cli.exe -db "%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem, StatusLog FROM CyborgNodes_Temp" -diag
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 6] FILESYSTEM RELOCATION TO SANDBOX VAULT
echo ----------------------------------------------------------------------
echo [MOVE] Shifting %DB_NAME% into %TEMP_DIR%\...
move /y "%DB_NAME%" "%TEMP_DIR%\%DB_NAME%" >nul
echo [FILESYSTEM] Active Vault Directory Listing:
dir /w /b "%TEMP_DIR%"
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 7] REMOTE QUERYING RELOCATED DATABASE CONTAINER
echo ----------------------------------------------------------------------
dc2sql_cli.exe -db "%TEMP_DIR%\%DB_NAME%" -query "SELECT NodeID, OperatorCode, Subsystem, StatusLog FROM CyborgNodes_Temp" -diag
echo.
echo.
echo.
echo ----------------------------------------------------------------------
echo [PHASE 8] SYSTEM TEARDOWN  REPOSITORY SANITIZATION
echo ----------------------------------------------------------------------
echo [CLEANUP] Dropping remote database tables...
dc2sql_cli.exe -db "%TEMP_DIR%\%DB_NAME%" -query "DROP TABLE CyborgNodes" -diag
dc2sql_cli.exe -db "%TEMP_DIR%\%DB_NAME%" -query "DROP TABLE CyborgNodes_Temp" -diag
echo [CLEANUP] Purging database container and vault directory...
del /f /q "%TEMP_DIR%\%DB_NAME%" >nul
rmdir "%TEMP_DIR%" >nul
echo.
echo.
echo.
echo ======================================================================
echo   STATUS: SUCCESS - ALL MATRIX LOOPS, -LANG  -ACK SWITCHES COMPLETED.
echo ======================================================================
endlocal
pause