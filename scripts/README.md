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
```
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
