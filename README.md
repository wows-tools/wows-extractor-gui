# wows-extractor-gui

Qt GUI for extracting/exporting World of Warships resource files and 3D models.

This application uses [wows-depack](https://github.com/wows-tools/wows-depack) and [wows-geometry](https://github.com/wows-tools/wows-geometry) (`wows-model-exporter` submodule) to enable browsing World of Warships game files.

It also allows you to preview 3D models of ships and export them in the more standard `.glTF`/`.glb` format.

## Known Issues

These are some issues I might solve someday, but don't hold your breath:

* Ships are mirrored (especially nameplates).
* Propellers are missing
* Torpedoes are missing

## Screenshots

![Resource Explorer](misc/resource_extractor.png)

![3D Model Viewer / Exporter](misc/3dmodel_viewer_exporter.png)

## Requirements

- CMake 3.16 or newer
- C++17 compiler
- Qt 6 (Core, Widgets, Quick, QuickWidgets, Quick3D)
- Zlib, PCRE2 (`libpcre2-8`), meshoptimizer, Python 3 development headers, TinyGLTF

## Clone

This repository uses Git submodules:

```bash
git clone --recurse-submodules https://github.com/wows-tools/wows-extractor-gui
cd wows-extractor-gui
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```


## Build (Linux, Debian/Ubuntu-style packages)

Install dependencies (names may differ slightly on other distributions):

```bash
sudo apt install \
  build-essential cmake ninja-build \
  qt6-base-dev qt6-declarative-dev qt6-quick3d-dev \
  zlib1g-dev libpcre2-dev libpython3-dev \
  libtinygltf-dev libmeshoptimizer-dev
```

Configure and build:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is written to `build/bin/wows-extractor`.

Install system-wide (optional):

```bash
cmake --install build
```

## Run

```bash
./build/bin/wows-extractor
```

## Build (Windows, MSVC)

### Prerequisites

- [Git for Windows](https://git-scm.com/download/win)
- [CMake](https://cmake.org/download/) 3.16+
- [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/) with the **Desktop development with C++** workload (MSVC + Windows SDK)
- [Python 3.11+](https://www.python.org/downloads/) (64-bit; the installer adds `include` and `libs` used by the model exporter)
- **Qt 6.7** for MSVC 64-bit with modules **Qt Quick 3D**, **Shader Tools**, and **Quick Timeline**

Qt is not provided by vcpkg. The CI workflow uses Qt **6.7.3** / `win64_msvc2019_64`. You can install it with [aqtinstall](https://github.com/miurahr/aqtinstall):

```powershell
pip install aqtinstall
python -m aqt install-qt windows desktop 6.7.3 win64_msvc2019_64 `
  -m qtquick3d qtshadertools qtquicktimeline -O C:\Qt
```

Native dependencies (zlib, PCRE2, meshoptimizer, tinygltf, mman, dirent) are fetched automatically via the **vcpkg** submodule on first configure.

### Clone and submodules

Use the same clone/submodule steps as above. On Windows, do **not** initialize `wows-model-exporter/deps/wows-depack` (that nested submodule contains paths illegal on Windows); the top-level `wows-depack` submodule is used instead:

```powershell
git submodule update --init wows-depack vcpkg wows-model-exporter
git -C wows-model-exporter submodule update --init deps/stb
```

### Configure and build

From a **Developer Command Prompt for VS 2022** (or any shell after running `vcvars64.bat`), in the repository root:

```powershell
$env:QT_ROOT = "C:\Qt\6.7.3\msvc2019_64"   # adjust if needed

cmake -B build -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="$PWD\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-md `
  -DCMAKE_PREFIX_PATH="$env:QT_ROOT" `
  -DPython3_ROOT_DIR="$env:LOCALAPPDATA\Programs\Python\Python311"

cmake --build build --config Release --parallel
```

Or use the helper script (finds Qt/Python when installed in default locations):

```powershell
.\scripts\build-windows.ps1 -QtRoot C:\Qt\6.7.3\msvc2019_64
```

The executable is `build\bin\wows-extractor.exe`.

To run from the build tree you still need Qt DLLs and QML modules on `PATH` (or next to the `.exe`). After `cmake --install`, use Qt’s `windeployqt` as in [`.github/workflows/windows.yml`](.github/workflows/windows.yml).

### MSI installer

Requires a successful build and [WiX Toolset 3.x](https://wixtoolset.org/) (`heat.exe`, `candle.exe`, `light.exe`). If the MSI installer is not installed system-wide, extract [wix314-binaries.zip](https://github.com/wixtoolset/wix3/releases/download/wix3141rtm/wix314-binaries.zip) and set `WIX314` to that folder.

```powershell
.\scripts\create-msi.ps1 -WixDir C:\path\to\wix314
```

Output: `wows-extractor-<version>-win64.msi` in the repository root (installs to `Program Files\WoWS Extractor` with a Start Menu shortcut).

## License

[GNU General Public License v3.0](LICENSE)

## Notes

- **Game data**: you need a local World of Warships installation or a copied game/res directory; the UI asks for the game installation directory (the **Game Directory** field at the top of the window).
- **Qt Quick 3D** uses OpenGL; ensure your graphics drivers support OpenGL 3.3 Core (or the profile your Qt build targets).
