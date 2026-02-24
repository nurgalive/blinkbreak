#!/usr/bin/env python3
"""Convert SVG files to multi-resolution ICO files.

Strategy:
  1. Use Node.js + sharp to rasterize SVGs to PNG at multiple sizes
  2. Use Pillow to pack the PNGs into a single ICO

If sharp is not installed globally, the script installs it into a
temporary node_modules directory under scripts/.

Usage:
    python convert_icons.py <in1.svg> <out1.ico> [<in2.svg> <out2.ico> ...]

Required: Node.js (>= 16), Python 3.10+, Pillow
"""

from __future__ import annotations

import json
import os
import struct
import subprocess
import sys
import tempfile
from io import BytesIO
from pathlib import Path


# The sizes to embed in the ICO file - largest first for Windows
ICON_SIZES = [256, 64, 48, 32, 24, 16]


def _ensure_pillow() -> None:
    """Install Pillow if not already available."""
    try:
        from PIL import Image  # noqa: F401
    except ImportError:
        print("Installing Pillow...")
        subprocess.check_call(
            [sys.executable, "-m", "pip", "install", "--quiet", "Pillow"]
        )


def _ensure_sharp(scripts_dir: Path) -> Path:
    """Ensure sharp is available and return the node_modules path."""
    import shutil

    node_modules = scripts_dir / "node_modules"
    sharp_dir = node_modules / "sharp"
    if not sharp_dir.exists():
        print("Installing sharp (one-time)...")
        npm_cmd = shutil.which("npm") or "npm"
        subprocess.check_call(
            [npm_cmd, "install", "--prefix", str(scripts_dir), "sharp"],
            cwd=str(scripts_dir),
            shell=(sys.platform == "win32"),
        )
    return node_modules


def rasterize_svg(svg_path: Path, sizes: list[int], scripts_dir: Path) -> list[bytes]:
    """Rasterize an SVG to multiple PNG byte buffers using Node.js + sharp.

    Returns a list of PNG byte strings, one per requested size.
    """
    _ensure_sharp(scripts_dir)

    # Build a small inline Node.js script
    js_code = """
const sharp = require('sharp');
const fs = require('fs');
const path = require('path');

(async () => {
    const svgPath = process.argv[2];
    const sizes = JSON.parse(process.argv[3]);
    const outDir = process.argv[4];

    const svgBuffer = fs.readFileSync(svgPath);

    for (const size of sizes) {
        // The SVG is naturally 400+ points wide, so standard density (72) is more 
        // than sufficient to yield a crisp 256x256 image after resizing down.
        const pngBuf = await sharp(svgBuffer)
            .resize(size, size, { fit: 'contain', background: { r: 0, g: 0, b: 0, alpha: 0 } })
            .png()
            .toBuffer();
        const outFile = path.join(outDir, `icon_${size}.png`);
        fs.writeFileSync(outFile, pngBuf);
    }
})();
"""

    with tempfile.TemporaryDirectory() as tmpdir:
        js_file = Path(tmpdir) / "rasterize.js"
        js_file.write_text(js_code, encoding="utf-8")

        # Set NODE_PATH so require('sharp') resolves
        node_modules = scripts_dir / "node_modules"
        env = {**os.environ, "NODE_PATH": str(node_modules)}

        subprocess.check_call(
            [
                "node",
                str(js_file),
                str(svg_path),
                json.dumps(sizes),
                tmpdir,
            ],
            env=env,
        )

        pngs: list[bytes] = []
        for size in sizes:
            png_path = Path(tmpdir) / f"icon_{size}.png"
            pngs.append(png_path.read_bytes())
        return pngs


def pack_ico(png_buffers: list[bytes], ico_path: Path) -> None:
    """Pack raw PNG byte buffers into an ICO file format."""
    # Build ICO header
    num_images = len(png_buffers)
    header = struct.pack("<hhh", 0, 1, num_images)
    
    # Directory entries and data
    directory_entries = bytearray()
    image_data = bytearray()
    
    # Offset starts after header (6 bytes) + directory entries (16 bytes each)
    current_offset = 6 + (16 * num_images)
    
    for buf in png_buffers:
        # Read PNG dimensions
        if not buf.startswith(b"\x89PNG\r\n\x1a\n"):
            raise ValueError("Not a valid PNG buffer")
        
        # Parse IHDR chunk
        width, height = struct.unpack(">II", buf[16:24])
        
        # ICO restricts width/height to 0-255 with 0 meaning 256
        w = 0 if width >= 256 else width
        h = 0 if height >= 256 else height
        
        size = len(buf)
        
        # entry length: 16 bytes
        # width, height, colors (0=256+), reserved, planes (1), bpp (32), size, offset
        entry = struct.pack("<BBBBHHII", w, h, 0, 0, 1, 32, size, current_offset)
        directory_entries.extend(entry)
        
        image_data.extend(buf)
        current_offset += size
        
    ico_path.parent.mkdir(parents=True, exist_ok=True)
    with open(ico_path, "wb") as f:
        f.write(header)
        f.write(directory_entries)
        f.write(image_data)

def convert_svg_to_ico(svg_path: Path, ico_path: Path, scripts_dir: Path) -> None:
    """Convert a single SVG to a multi-resolution ICO."""
    png_buffers = rasterize_svg(svg_path, ICON_SIZES, scripts_dir)
    pack_ico(png_buffers, ico_path)
    size_str = ", ".join(f"{s}x{s}" for s in ICON_SIZES)
    print(f"  {svg_path.name} -> {ico_path.name}  ({size_str})")


def main() -> int:
    if len(sys.argv) < 3 or len(sys.argv) % 2 != 1:
        print(
            f"Usage: {sys.argv[0]} <input1.svg> <output1.ico>"
            " [<input2.svg> <output2.ico> ...]"
        )
        return 1

    _ensure_pillow()

    scripts_dir = Path(__file__).resolve().parent

    args = sys.argv[1:]
    pairs = [(args[i], args[i + 1]) for i in range(0, len(args), 2)]

    print(f"Converting {len(pairs)} icon(s)...")
    for svg_str, ico_str in pairs:
        svg_path = Path(svg_str).resolve()
        ico_path = Path(ico_str).resolve()

        if not svg_path.exists():
            print(f"ERROR: SVG file not found: {svg_path}", file=sys.stderr)
            return 1

        convert_svg_to_ico(svg_path, ico_path, scripts_dir)

    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
