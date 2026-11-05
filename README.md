# Mercwar AI Software — `dc2sql` & `dc2jet` Suite

An enterprise-grade, administrative-elevated compilation, packaging, and digital signing pipeline designed for the **`dc2sql`** database migration and translation architecture. This suite automates resource compilation, security catalog generation (`.cdf` / `.cat`), certificate provisioning, and code-signing routines for Windows native binaries.

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

---

## Security & Signing Workflow

1. **Environment Elevation:** Verifies administrator privileges and re-launches with elevated tokens if required.
2. **Target Discovery:** Parses the target INI manifest and normalizes relative paths against the configuration working directory.
3. **Catalog Construction (`makecat`):** Automatically generates a clean `.cdf` definition file containing cryptographic file hashes (`SHA256`) and invokes the Windows SDK `makecat.exe` utility to compile the security catalog (`.cat`).
4. **Certificate Provisioning:** Generates a self-signed code-signing certificate (or uses an existing one) and imports it into both the `Root` and `My` certificate stores using `certutil`.
5. **Authenticode Signing (`signtool`):** Applies an SHA-256 signature alongside a trusted time-stamping authority (`[http://timestamp.digicert.com](http://timestamp.digicert.com)`) to both the security catalog and individual binaries (`.dll`, `.exe`).
