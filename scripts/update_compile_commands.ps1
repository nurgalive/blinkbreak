# update_compile_commands.ps1
# Post-build script: copies ninja-debug compile_commands.json to the project root
# and patches it for clangd compatibility:
#   1. Replaces cl.exe with clang-cl.exe so clangd respects -std=c++ properly
#   2. Converts dash-prefixed MSVC flags to slash-prefixed (clang-cl understands /)
#   3. Converts -external:I paths to plain -I paths
#
# Run after: cmake --build --preset ninja-debug

$ErrorActionPreference = "Stop"

$root    = Split-Path $PSScriptRoot -Parent
$src     = Join-Path $root "build\ninja-debug\compile_commands.json"
$dst     = Join-Path $root "compile_commands.json"
$clangCl = "C:\Program Files\LLVM\bin\clang-cl.exe"
$msvcCl  = 'C:\\PROGRA~1\\MIB055~1\\2022\\PROFES~1\\VC\\Tools\\MSVC\\1444~1\.352\\bin\\Hostx64\\x64\\cl\.exe'

if (-not (Test-Path $src)) {
    Write-Error "Source not found: $src -- run 'cmake --build --preset ninja-debug' first"
    exit 1
}

$content = Get-Content $src -Raw

# 1. Replace MSVC cl.exe with clang-cl
$clangClEscaped = $clangCl -replace '\\', '\\\\'
$content = $content -replace $msvcCl, $clangClEscaped

# 2. Convert -std:c++latest / -std:c++23 to /std:c++23
$content = $content -replace ' -std:c\+\+latest ', ' /std:c++23 '
$content = $content -replace ' -std:c\+\+23 ',    ' /std:c++23 '

# 3. Convert debug/runtime dash-flags to slash-prefixed (clang-cl compatible)
$content = $content -replace ' -MDd ',  ' /MDd '
$content = $content -replace ' -MDd"',  ' /MDd"'
$content = $content -replace ' -Zi ',   ' /Zi '
$content = $content -replace ' -Zi"',   ' /Zi"'

# 4. Convert -external:I paths to plain -I (preserve include paths, drop the flag prefix)
$content = $content -replace '-external:I', '-I'
$content = $content -replace ' -external:W0', ''
$content = $content -replace ' -external:W0"', '"'

# 5. Write patched copy to the project root
Set-Content $src $content
Copy-Item $src $dst -Force

Write-Host "compile_commands.json updated and patched for clangd."
