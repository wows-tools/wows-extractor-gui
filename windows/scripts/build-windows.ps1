#Requires -Version 5.1
<#
.SYNOPSIS
  Configure and build wows-extractor-gui on Windows (MSVC + vcpkg + Qt 6).

.PARAMETER QtRoot
  Path to the Qt MSVC kit directory (e.g. C:\Qt\6.7.3\msvc2019_64).

.PARAMETER PythonRoot
  Path to a Python 3 install with development headers (include + libs).

.PARAMETER BuildType
  CMake build type (default: Release).

.PARAMETER ConfigureOnly
  Run cmake configure only; do not build.

.EXAMPLE
  .\scripts\build-windows.ps1 -QtRoot C:\Qt\6.7.3\msvc2019_64
#>
[CmdletBinding()]
param(
    [string]$QtRoot = $env:QT_ROOT,
    [string]$PythonRoot = $env:Python3_ROOT_DIR,
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$BuildType = 'Release',
    [switch]$ConfigureOnly
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Set-Location $RepoRoot

function Find-VcVars64 {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        throw 'vswhere.exe not found. Install Visual Studio 2022 Build Tools with the C++ workload.'
    }
    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $installPath) {
        throw 'No MSVC installation found. Install Visual Studio 2022 Build Tools with the C++ workload.'
    }
    $vcvars = Join-Path $installPath 'VC\Auxiliary\Build\vcvars64.bat'
    if (-not (Test-Path $vcvars)) { throw "vcvars64.bat not found under $installPath" }
    return $vcvars
}

function Ensure-Submodules {
    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) {
        $git = 'C:\Program Files\Git\cmd\git.exe'
        if (-not (Test-Path $git)) { throw 'git not found on PATH' }
    } else {
        $git = $git.Source
    }
    & $git submodule update --init wows-depack vcpkg wows-model-exporter
    if ($LASTEXITCODE -ne 0) { throw 'git submodule update failed (wows-depack, vcpkg, wows-model-exporter)' }
    & $git -C wows-model-exporter submodule update --init deps/stb
    if ($LASTEXITCODE -ne 0) { throw 'git submodule update failed (wows-model-exporter/deps/stb)' }
}

function Resolve-QtRoot {
    param([string]$Hint)
    if ($Hint -and (Test-Path (Join-Path $Hint 'lib\cmake\Qt6'))) { return (Resolve-Path $Hint).Path }
    $candidates = @(
        $env:QT_ROOT,
        $env:QT_ROOT_DIR,
        'C:\Qt\6.7.3\msvc2019_64',
        "$env:USERPROFILE\Qt\6.7.3\msvc2019_64"
    ) | Where-Object { $_ }
    foreach ($c in $candidates) {
        if (Test-Path (Join-Path $c 'lib\cmake\Qt6')) { return (Resolve-Path $c).Path }
    }
    throw @'
Qt 6 (MSVC 64-bit) not found. Install Qt 6.7.x with modules qtquick3d, qtshadertools, qtquicktimeline, then pass -QtRoot:

  pip install aqtinstall
  python -m aqt install-qt windows desktop 6.7.3 win64_msvc2019_64 `
    -m qtquick3d qtshadertools qtquicktimeline -O C:\Qt

  .\windows\scripts\build-windows.ps1 -QtRoot C:\Qt\6.7.3\msvc2019_64
'@
}

function Resolve-PythonRoot {
    param([string]$Hint)
    if ($Hint -and (Test-Path (Join-Path $Hint 'include\Python.h'))) { return (Resolve-Path $Hint).Path }
    $py = Get-Command python -ErrorAction SilentlyContinue
    if ($py) {
        $root = & $py -c "import sys; print(sys.base_prefix)"
        if (Test-Path (Join-Path $root 'include\Python.h')) { return $root }
    }
    $fallback = "$env:LOCALAPPDATA\Programs\Python\Python311"
    if (Test-Path (Join-Path $fallback 'include\Python.h')) { return $fallback }
    throw 'Python 3 with development headers not found. Install Python 3.11+ from python.org and pass -PythonRoot.'
}

Ensure-Submodules
$QtRoot = Resolve-QtRoot -Hint $QtRoot
$PythonRoot = Resolve-PythonRoot -Hint $PythonRoot
$VcVars = Find-VcVars64

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) { throw 'cmake not found on PATH' }

$toolchain = (Join-Path $RepoRoot 'vcpkg\scripts\buildsystems\vcpkg.cmake') -replace '\\', '/'
$qt = $QtRoot -replace '\\', '/'
$py = $PythonRoot -replace '\\', '/'

$configureArgs = @(
    '-B', 'build',
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    '-DVCPKG_TARGET_TRIPLET=x64-windows-static-md',
    "-DCMAKE_PREFIX_PATH=$qt",
    "-DPython3_ROOT_DIR=$py"
)

Write-Host "==> Configuring ($BuildType) ..."
$configureCmd = "`"$VcVars`" && cd /d `"$RepoRoot`" && `"$($cmake.Source)`" $($configureArgs -join ' ')"
cmd /c $configureCmd
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed (exit $LASTEXITCODE)" }

if ($ConfigureOnly) { return }

Write-Host "==> Building ..."
$buildCmd = "`"$VcVars`" && cd /d `"$RepoRoot`" && `"$($cmake.Source)`" --build build --config $BuildType --parallel"
cmd /c $buildCmd
if ($LASTEXITCODE -ne 0) { throw "cmake build failed (exit $LASTEXITCODE)" }

$exe = Join-Path $RepoRoot "build\bin\wows-extractor.exe"
if (-not (Test-Path $exe)) {
  # Pre-MSVC-output-dir fix or older build trees may still use bin\Release\
  $exe = Join-Path $RepoRoot "build\bin\$BuildType\wows-extractor.exe"
}
if (Test-Path $exe) {
    Write-Host "Built: $exe"
} else {
    Write-Warning "Build finished but wows-extractor.exe was not found under build\bin\"
}
