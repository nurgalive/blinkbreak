# Scripts

This directory contains utility scripts that support the development and build processes of BlinkBreak.

## Icon Conversion

The `convert_icons.py` script is used to convert vector SVG graphics into multi-resolution `.ico` files containing crisp rasterized bitmaps for use by Windows and the Slint UI framework.

### Why not just use Pillow?

Python's `Pillow` library has limitations when attempting to save multi-resolution ICO files, particularly issues relating to blurry downscaling or upscaling if not passed perfect frame inputs.

To guarantee that the high-resolution taskbar and system tray icons remain completely crisp across all monitor DPI settings, the conversion script adopts a hybrid approach:

1. It automatically delegates the core SVG rasterization to a locally injected **Node.js** instance running the highly-optimized **sharp** library. `sharp` effectively calculates and renders exact pixel-perfect `icon_{size}.png` frames at the required target dimensions without applying destructive interpolations.
2. The script then manually packages those individual high-resolution PNG chunks directly into a raw byte-compliant `.ico` structure.

### Usage

```powershell
# Convert SVG resources into ICOs
python convert_icons.py <input.svg> <output.ico>

Example:

```powershell
python convert_icons.py ../resources/bb_logo_notext_blue.svg ../resources/bb_logo_blue.ico
```

The script specifically packages the icons at standard geometric sizes `[256, 64, 48, 32, 24, 16]` for optimal Windows integration.

### Prerequisites

- **Python 3.10+**
- **Node.js (>= 16)**
- **Pillow** (`pip install Pillow`)

### Manual Node Module Installation

While the `convert_icons.py` script is designed to automatically bootstrap its own Node dependencies into an isolated `node_modules` folder, you can also install them manually if preferred:

```powershell
# Navigate to the scripts directory
cd scripts

# Install dependencies defined in package.json
npm install
```

This will install the `sharp` library required for SVG rasterization.

## Utilities

- `dump_ico.py`: A little inspection utility script to output and verify the layers, frame counts, and sizing embedded within a generated `.ico` package without needing a hex editor.

## Memory Analysis

Use `run_drmemory.ps1` to run a portable Dr. Memory scan against the BlinkBreak executable without installing anything system-wide.

### Usage

```powershell
# Baseline startup/shutdown check (default: runs blinkbreak.exe --version)
.\scripts\run_drmemory.ps1

# Configure + build first, then scan
.\scripts\run_drmemory.ps1 -BuildFirst

# Scan the full app instead of the non-interactive --version path
.\scripts\run_drmemory.ps1 -AppArgs @()

# Force a fresh Dr. Memory download/extract
.\scripts\run_drmemory.ps1 -Redownload
```

### Guidelines

- Use the default invocation first because `--version` exercises startup/shutdown cleanly and exits on its own.
- Use `-AppArgs @()` only when you want to inspect the interactive UI path and are ready to close the app manually.
- Dr. Memory returns a non-zero exit code when it finds errors because the script uses `-exit_code_if_errors 1`.
- Reports are written under `tmp/drmemory-results`, and the script prints the newest `results.txt` path after each run.
- The first run on a new Windows build may auto-generate syscall metadata before re-launching the target.
