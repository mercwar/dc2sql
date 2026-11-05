<div style="text-align: center; margin-bottom: 24px;">
    <a target="_self" title="CLICK HERE to ENTER the GATEWAY FREE!" href="https://mercwar.github.io/Constellation/index.html">
    <img 
        src="https://raw.githubusercontent.com/mercwar/Robo-Knight-Gallery/refs/heads/main/Version%207/image_d2a07390.png" 
        alt="Mercwar Constellation - Gateway Access" 
        style="width:100%; height:auto; border-radius:12px; box-shadow:0 6px 16px rgba(0,0,0,0.6);"
    />
    </a>
</div>

# Mercwar AI Software — `dc2sql` & `dc2jet` Suite
An enterprise-grade, administrative-elevated compilation, packaging, and digital signing pipeline designed for the **`dc2sql`** database migration and translation architecture. This suite automates resource compilation, security catalog generation (`.cdf` / `.cat`), certificate provisioning, and code-signing routines for Windows native binaries.


![dc2](images/dc2sql-2.png)
## Architecture Summary: Bridging File Drives to SQL

The **`dc2sql` and `dc2jet` suite** functions as a high-performance database bridge and SQL router. It eliminates the friction of legacy file-based databases—such as Microsoft Access (`.mdb` / `.accdb`) or Microsoft Jet engine files stored on local disks, network shares, or cloud-synced folders—by translating standard SQL queries directly into native DAO (Data Access Objects) operations.

Instead of rewriting legacy applications to use a heavy client-server database engine, `dc2sql` allows modern software to talk to file-based data stores using standard relational SQL syntax, reading directly from any accessible drive or path.

---

## How the Database Connects to a Drive

The connection mechanism relies on direct path resolution and runtime file mapping handled by `dc2jet.dll` and `dc2sql.dll`:

1. **Path Resolution:** The application points to a physical or network file path where your database file resides.
2. **Engine Initialization (`dc2jet.dll`):** The Microsoft Jet database engine is loaded in-process, attaching securely to the specified file path via DAO/ODBC providers.
3. **SQL Translation (`dc2sql.dll`):** Incoming SQL queries (SELECT, INSERT, UPDATE, DELETE) are intercepted, optimized, and converted into native Jet/DAO commands executed against the file on disk.
4. **Result Streaming:** Records are piped back through the router to the calling application as clean standard result sets.

---

## Practical Examples

### Example 1: Connecting to a Local Drive Path

If your legacy inventory database (`inventory.accdb`) is stored on a local workspace drive, `dc2sql` mounts the file path directly for querying:

```sql
-- Connecting and querying an Access database on a local drive
USE [C:\Mercwar-AI\data\inventory.accdb];

SELECT ProductID, ProductName, UnitPrice 
FROM Products 
WHERE UnitsInStock < 10;

```

* **How it works:** The engine reads the `.accdb` file directly from the `C:\` drive path, executes the filter natively inside the Jet database engine, and returns low-stock items instantly without needing a running database service (like SQL Server or MySQL).

---

### Example 2: Connecting to a Network Shared Drive (`UNC Path`)

For enterprise environments where legacy databases live on centralized network attached storage (NAS) or file servers, `dc2sql` resolves Universal Naming Convention (UNC) paths seamlessly:

```sql
-- Connecting to a centralized network share
USE [\\CorpFileServer01\Finance\LegacyLedger.mdb];

SELECT Department, SUM(Amount) AS TotalSpent 
FROM Expenses 
GROUP BY Department;

```

* **How it works:** Multiple client applications can route queries through `dc2sql_cli.exe` or `dc2sql.dll` targeting the shared network path, allowing shared access to file-backed data without locking conflicts or complex database cluster configurations.
```cmd
dc2sql_cli.exe --config C:\Mercwar-AI\dc2sql-config.json --new-cert

```

![dc2](images/dc2sql-4.png)



###### *"<i>I am CVBGOD, and I have given it to you</i>!"*

---


## Architecture Overview

The pipeline consists of a robust C-based runner utility (`dc2sql_cli.exe`) paired with core database provider binaries and helper dynamic-link libraries:

```text
C:\Mercwar-AI\
│
├── dc2sql-config.json      # Centralized deployment configuration
├── dc2sql_targets.ini      # Target asset manifest manifest
├── dc2sql.pfx              # Code-signing digital certificate
├── dc2sql.cat              # Cryptographic security catalog
├── dc2sql.cdf              # Catalog definition file
│
└── dc2sql\                 # Target Binaries Directory
    ├── dc2jet.dll          # Microsoft Jet / Database bridge component
    ├── dc2sql.dll          # Core translation and translation core library
    └── dc2sql_cli.exe      # Primary command-line interface & signing engine

```



---

## Components & Manifests

### 1. Configuration Matrix (`dc2sql-config.json`)

Controls paths, cryptographic parameters, timestamp servers, and execution behaviors:

```json
{
  "Config": {
    "Title": "Mercwar AI Software - dc2sql & dc2jet Signing Pipeline",
    "TimestampUrl": "http://timestamp.digicert.com",
    "cert_cat": "C:\\Mercwar-AI\\dc2sql.cat",
    "cert_ini": "C:\\Mercwar-AI\\dc2sql_targets.ini",
    "cert_cdf": "C:\\Mercwar-AI\\dc2sql.cdf",
    "cert_reset": "yes",
    "cert_name": "C:\\Mercwar-AI\\dc2sql.pfx",
    "Subject": "CN=Mercwar AI Code Signing Authority",
    "Password": "37h653e7h63eh6e7j6"
  }
}

```

### 2. Target Manifest (`dc2sql_targets.ini`)

Declares all PE files evaluated and signed by the execution engine:

```ini
[TargetFiles]
File1=dc2sql\dc2jet.dll
File2=dc2sql\dc2sql.dll
File3=dc2sql\dc2sql_cli.exe

```

---

## Compilation & Resource Manifests

![dc2](images/dc2sql-1.png)

Each component includes dedicated resource scripts (`.rc`) embedding PE version metadata, company info, and application icons:

* **`dc2sql_cli.rc`** — Configures executable file metadata (`VFT_APP`), product versioning (`1.1.0.0`), and descriptive legal blocks.
* **`dc2sql_dll.rc`** — Dynamically mapped for provider libraries (`VFT_DLL`), ensuring proper distinction between core provider modules (`dc2sql.dll` and `dc2jet.dll`).

---

## Execution & Command-Line Reference

The utility automatically requests **Administrative Elevation** via UAC (`runas`) if launched under a standard user context.

### Command Usage

```cmd
dc2sql_cli.exe --config <path_to_json> [switches]

```

### Available Switches

* `--config <path>` — Specify the absolute or relative path to the deployment JSON configuration.
* `--new-cert` — Force-generate and register a fresh PKCS#12 (`.pfx`) test/production certificate asset.
* `--use-existing` — Bypass generation and utilize an existing `.pfx` file present on disk.
* `--help`, `-h` — Display the CLI reference guide and parameter descriptions.

### Example Execution




```cmd
dc2sql_cli.exe --config C:\Mercwar-AI\dc2sql-config.json --new-cert

```
## Architecture Summary: Bridging File Drives to SQL

The **`dc2sql` and `dc2jet` suite** functions as a high-performance database bridge and SQL router. It eliminates the friction of legacy file-based databases—such as Microsoft Access (`.mdb` / `.accdb`) or Microsoft Jet engine files stored on local disks, network shares, or cloud-synced folders—by translating standard SQL queries directly into native DAO (Data Access Objects) operations.

Instead of rewriting legacy applications to use a heavy client-server database engine, `dc2sql` allows modern software to talk to file-based data stores using standard relational SQL syntax, reading directly from any accessible drive or path.

---

## How the Database Connects to a Drive

The connection mechanism relies on direct path resolution and runtime file mapping handled by `dc2jet.dll` and `dc2sql.dll`:

1. **Path Resolution:** The application points to a physical or network file path where your database file resides.
2. **Engine Initialization (`dc2jet.dll`):** The Microsoft Jet database engine is loaded in-process, attaching securely to the specified file path via DAO/ODBC providers.
3. **SQL Translation (`dc2sql.dll`):** Incoming SQL queries (SELECT, INSERT, UPDATE, DELETE) are intercepted, optimized, and converted into native Jet/DAO commands executed against the file on disk.
4. **Result Streaming:** Records are piped back through the router to the calling application as clean standard result sets.

---

## Practical Examples

![dc2](images/dc2sql-3.png)

### Example 1: Connecting to a Local Drive Path

If your legacy inventory database (`inventory.accdb`) is stored on a local workspace drive, `dc2sql` mounts the file path directly for querying:

```sql
-- Connecting and querying an Access database on a local drive
USE [C:\Mercwar-AI\data\inventory.accdb];

SELECT ProductID, ProductName, UnitPrice 
FROM Products 
WHERE UnitsInStock < 10;

```

* **How it works:** The engine reads the `.accdb` file directly from the `C:\` drive path, executes the filter natively inside the Jet database engine, and returns low-stock items instantly without needing a running database service (like SQL Server or MySQL).

---

### Example 2: Connecting to a Network Shared Drive (`UNC Path`)

For enterprise environments where legacy databases live on centralized network attached storage (NAS) or file servers, `dc2sql` resolves Universal Naming Convention (UNC) paths seamlessly:

```sql
-- Connecting to a centralized network share
USE [\\CorpFileServer01\Finance\LegacyLedger.mdb];

SELECT Department, SUM(Amount) AS TotalSpent 
FROM Expenses 
GROUP BY Department;

```

* **How it works:** Multiple client applications can route queries through `dc2sql_cli.exe` or `dc2sql.dll` targeting the shared network path, allowing shared access to file-backed data without locking conflicts or complex database cluster configurations.
---

## Security & Signing Workflow

1. **Environment Elevation:** Verifies administrator privileges and re-launches with elevated tokens if required.
2. **Target Discovery:** Parses the target INI manifest and normalizes relative paths against the configuration working directory.
3. **Catalog Construction (`makecat`):** Automatically generates a clean `.cdf` definition file containing cryptographic file hashes (`SHA256`) and invokes the Windows SDK `makecat.exe` utility to compile the security catalog (`.cat`).
4. **Certificate Provisioning:** Generates a self-signed code-signing certificate (or uses an existing one) and imports it into both the `Root` and `My` certificate stores using `certutil`.
5. **Authenticode Signing (`signtool`):** Applies an SHA-256 signature alongside a trusted time-stamping authority (`[http://timestamp.digicert.com](http://timestamp.digicert.com)`) to both the security catalog and individual binaries (`.dll`, `.exe`).
