import json
import os
import sys


def main():
    # Resolve project root (parent directory of 'scripts')
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(script_dir)

    src_path = os.path.join(root_dir, "build", "ninja-debug", "compile_commands.json")
    dst_path = os.path.join(root_dir, "compile_commands.json")
    clang_cl = r"C:\Program Files\LLVM\bin\clang-cl.exe"

    # Fallback to existing file if build dir is missing
    if not os.path.exists(src_path):
        if os.path.exists(dst_path):
            print(f"Warning: Source not found: {src_path}. Using existing {dst_path}.")
            src_path = dst_path
        else:
            print(
                f"Error: Source not found: {src_path}\nRun 'cmake --preset ninja-debug' first"
            )
            sys.exit(1)

    with open(src_path, "r", encoding="utf-8") as f:
        compile_commands = json.load(f)

    for entry in compile_commands:
        if "command" in entry:
            cmd = entry["command"]

            # 1. Replace MSVC cl.exe with clang-cl.exe securely
            # Find the first space to isolate the executable path
            parts = cmd.split(" ", 1)
            if parts[0].lower().endswith("cl.exe") or parts[0].lower().endswith(
                'cl.exe"'
            ):
                rest_of_cmd = parts[1] if len(parts) > 1 else ""
                cmd = f'"{clang_cl}" {rest_of_cmd}'

            # 2. Convert MSVC -external:I paths to plain -I
            cmd = cmd.replace("-external:I", "-I")

            # 3. Strip -external:W0 flag
            cmd = cmd.replace(" -external:W0", "")

            entry["command"] = cmd

    # Write the patched database to the project root
    with open(dst_path, "w", encoding="utf-8") as f:
        json.dump(compile_commands, f, indent=2)

    print("compile_commands.json updated and patched securely for clangd via Python.")


if __name__ == "__main__":
    main()
