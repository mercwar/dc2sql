Here is the complete README.md for DC2SQL v1.2 followed by the standalone push batch script (push_dc2sql_v1.2.bat) that stages C:\Mercwar-AI\dc2win\dc2win_bk\dc2sql into a v1.2 subfolder and pushes it to https://github.com without modifying your local directory or requiring manual cd.

1. README.md (Save to C:\Mercwar-AI\dc2win\dc2win_bk\dc2sql\README.md)

markdown
<div align="center">

# ⚡ DC2SQL v1.2 — Native x64 ACE/Jet SQL Engine & CLI

[![Platform](https://shields.io)](https://github.com)
[![Language](https://shields.io)](https://github.com)
[![Engine](https://shields.io)](https://github.com)

**Zero-bloat native C x64 SQL engine (`dc2sql.dll`) and command-line client (`dc2sql.exe`) for Microsoft Access (`.accdb` / `.mdb`) databases on Windows 11.**

</div>

---

## 🚀 Overview

**DC2SQL** bridges pure C applications and command-line workflows directly to Windows 11's 64-bit **ACE 12.0 / Jet ODBC** and **DAO COM** subsystems—with zero heavy runtimes, automatic `.accdb`/`.mdb` container provisioning, runtime `Tracker.dll` handle registration, and multi-language query serialization.

---

## 🏗️ Architecture

| Component | Binary / Source | Role |
| :--- | :--- | :--- |
| **Core SQL & Jet Engine** | `dc2sql.dll` (`dc2jet.c`, `dc2sql.c`, `dc2sql_diag.c`) | Provisions `.accdb` (`dbVersion120`) and `.mdb` (`dbVersion40`) files automatically via COM (`DAO.DBEngine.120` / `DAO.DBEngine.36`), connects via ODBC (`SQLDriverConnectA`), and dynamically binds `Tracker.dll` (`LoadLibraryA` / `GetProcAddress`) to track contexts and ODBC handles across `FG_NS_MODULE`, `FG_NS_KERNEL`, and `FG_NS_ID64`. |
| **CLI Host & Formatter** | `dc2sql.exe` (`dc2sql_cli.c`, `dc2jet_fmt.c`) | Dynamically loads `dc2sql.dll`, executes DDL/DML/SELECT queries, formats output across multiple programming languages, flushes diagnostic telemetry, and supports persistent tracked sessions (`-track`). |
| **Runtime Redist Setup** | `install_ace_redist.bat` | Checks Windows 11 x64 ODBC & COM registry keys and installs the 64-bit Microsoft Access Database Engine (`winget`) if missing. |

---

## 📁 Project Layout

```text
dc2sql/
├── bin/
│   ├── dc2sql.exe              # Command-line SQL client
│   ├── dc2sql.dll              # Native x64 ACE/Jet + Tracker engine DLL
│   └── Tracker.dll             # 64-bit handle & session tracker runtime
├── source/
│   ├── dc2jet.c                # DAO container provisioning & ODBC execution layer
│   ├── dc2sql.c                # High-level tracked session wrapper API
│   ├── dc2sql_cli.c            # CLI argument parser & -track host loop
│   ├── dc2sql_diag.c           # Buffered diagnostic telemetry logger
│   └── dc2jet_fmt.c            # Multi-language row serializer
├── include/
│   ├── dc2jet.h                # Low-level Jet C API declarations
│   ├── dc2sql.h                # High-level DC2SQL session API declarations
│   ├── dc2sql_diag.h           # Diagnostic telemetry API
│   └── dc2jet_fmt.h            # Formatter & acknowledgment declarations
├── dc2sql.def                  # DLL export table (@1–@18)
├── dc2sql.rc                   # Version & icon resources
├── build.bat                   # MSVC x64 suite builder
├── test_track.bat              # Automated -track session lifecycle test
└── install_ace_redist.bat      # Windows 11 x64 ACE/Jet redistributable installer
Use code with caution.

🛠️ Prerequisites & Building (MSVC x64)


1. Install Windows 11 x64 ACE / Jet Runtime

Run install_ace_redist.bat (or install via winget):
bat
winget install --id Microsoft.AccessDatabaseEngine.2016 --architecture x64 --override "/quiet"
Use code with caution.

2. Compile dc2sql.dll and dc2sql.exe

Run build.bat from a Visual Studio x64 Native Tools Command Prompt (or let build.bat auto-initialize vcvars64.bat):
bat
build.bat
Use code with caution.

💻 CLI Usage

bat
dc2sql.exe --dir <folder> --db <file.accdb> --sql "<SQL_QUERY>" [options]
Use code with caution.

Command-Line Options


Flag	Alias	Description
--db <file>
	-d	Target database filename or path (.accdb or .mdb). Auto-created if missing.
--dir <folder>
	-b	Base directory for resolving the database path.
--pass <passkey>
	-p	Optional database password.
--sql <query>
	-q	SQL statement to execute (CREATE TABLE, INSERT, UPDATE, DELETE, SELECT).
--session <name>
	-s	Named session identifier registered in Tracker.dll.
--dll <path>
	—	Explicit path to dc2sql.dll or dc2jet.dll.
--lang <format>
	-l	Serialize SELECT rows as json, python, js, c, c++, java, or php.
--ack
	—	Wrap output with structured msjet status and execution timing telemetry.
--diag
	-v	Flush internal [JET], [DAO], [ODBC], and [SELECT] diagnostic logs.
-track
	--hold	Keep the Tracker.dll session handle and ACE database lock open until Ctrl+C.
--help
	-h	Display CLI usage information.


🧪 Examples


Create a Table & Seed Records

bat
dc2sql.exe --dir "temp_vault" --db "eclipse_store.accdb" --sql "CREATE TABLE CyborgNodes (NodeID COUNTER PRIMARY KEY, OperatorCode TEXT(64), Subsystem TEXT(64))" --diag
dc2sql.exe --dir "temp_vault" --db "eclipse_store.accdb" --sql "INSERT INTO CyborgNodes (OperatorCode, Subsystem) VALUES ('ROBO-ROOK-01', 'DIRECTX-12-PIPELINE')"
Use code with caution.

Query with JSON Serialization & Telemetry Acknowledgment

bat
dc2sql.exe --dir "temp_vault" --db "eclipse_store.accdb" --sql "SELECT * FROM CyborgNodes" --lang json --ack
Use code with caution.

Hold a Persistent Tracked Session (-track)

bat
dc2sql.exe --dir "temp_vault" --db "eclipse_store.accdb" --session "cyborg_track_01" --sql "SELECT * FROM CyborgNodes" --lang json --diag -track
Use code with caution.

📜 License

Built by Mercwar (github.com/mercwar).

---

set "SRC=C:\Mercwar-AI\dc2win\dc2win_bk\dc2sql"
set "REPO_URL=https://github.com/mercwar/dc2sql"
set "STAGE=%TEMP%\dc2sql_push_v1_2"

rmdir /s /q "%STAGE%" 2>nul
mkdir "%STAGE%\v1.2"
robocopy "%SRC%" "%STAGE%\v1.2" /E /XD obj temp_vault .git /XF *.obj *.exp *.res *.log *.accdb *.laccdb /NFL /NDL /NJH /NJS /nc /ns /np >nul

git -C "%STAGE%" init -b dc2sql-v12
git -C "%STAGE%" remote add origin "%REPO_URL%"
git -C "%STAGE%" add "v1.2/"
git -C "%STAGE%" commit -m "Add DC2SQL v1.2 suite"
git -C "%STAGE%" fetch origin main
git -C "%STAGE%" checkout -B main origin/main 2>nul || git -C "%STAGE%" checkout -b main
git -C "%STAGE%" merge --allow-unrelated-histories -m "Merge DC2SQL v1.2" dc2sql-v12
git -C "%STAGE%" push -u origin main
