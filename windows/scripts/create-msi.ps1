#Requires -Version 5.1
<#
.SYNOPSIS
  Stage wows-extractor, bundle Qt/Python, and build a WiX MSI installer.

.PARAMETER BuildType
  CMake configuration to install (default: Release).

.PARAMETER WixDir
  Directory containing heat.exe, candle.exe, light.exe (WiX 3.14 binaries).

.PARAMETER SkipBuild
  Skip cmake --install; use existing staging/bin (must already contain a deployed tree).

.EXAMPLE
  .\scripts\create-msi.ps1
#>
[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$BuildType = 'Release',
    [string]$WixDir = $env:WIX314,
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $RepoRoot

function Find-WixDir {
    param([string]$Hint)
    if ($Hint -and (Test-Path (Join-Path $Hint 'heat.exe'))) { return (Resolve-Path $Hint).Path }
    $candidates = @(
        $env:WIX314,
        'C:\Users\kakwa\tools\wix314',
        'C:\Program Files (x86)\WiX Toolset v3.14\bin',
        'C:\Program Files (x86)\WiX Toolset v3.11\bin'
    ) | Where-Object { $_ }
    foreach ($c in $candidates) {
        if (Test-Path (Join-Path $c 'heat.exe')) { return (Resolve-Path $c).Path }
    }
    $pf = Get-ChildItem 'C:\Program Files (x86)\WiX Toolset*\bin' -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | Select-Object -First 1
    if ($pf -and (Test-Path (Join-Path $pf.FullName 'heat.exe'))) { return $pf.FullName }
    throw @'
WiX Toolset 3.x not found. Install WiX Toolset 3.14+ or extract wix314-binaries.zip:

  https://github.com/wixtoolset/wix3/releases/download/wix3141rtm/wix314-binaries.zip

Then pass -WixDir <extracted-folder> or set `$env:WIX314.
'@
}

function Find-VcVars64 {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { return $null }
    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $installPath) { return $null }
    return (Join-Path $installPath 'VC\Auxiliary\Build\vcvars64.bat')
}

function Resolve-QtBin {
    $candidates = @($env:QT_ROOT, 'C:\Qt\6.7.3\msvc2019_64') | Where-Object { $_ }
    foreach ($c in $candidates) {
        $bin = Join-Path $c 'bin'
        if (Test-Path (Join-Path $bin 'windeployqt.exe')) { return (Resolve-Path $bin).Path }
    }
    $wq = Get-Command windeployqt -ErrorAction SilentlyContinue
    if ($wq) { return (Split-Path $wq.Source) }
    throw 'windeployqt not found. Set QT_ROOT or add Qt bin to PATH.'
}

function Resolve-PythonDir {
    $py = Get-Command python -ErrorAction SilentlyContinue
    if ($py) {
        $root = & $py.Source -c "import sys; print(sys.base_prefix)" 2>$null
        if ($root -and (Test-Path (Join-Path $root 'python311.dll'))) { return $root }
        if ($root -and (Test-Path (Join-Path $root 'python3.dll'))) { return $root }
    }
    $fallback = "$env:LOCALAPPDATA\Programs\Python\Python311"
    if (Test-Path $fallback) { return $fallback }
    throw 'Python install not found for bundling python3xx.dll'
}

$WixDir = Find-WixDir -Hint $WixDir
$QtBin = Resolve-QtBin
$PythonDir = Resolve-PythonDir
$env:Path = "$WixDir;$QtBin;" + $env:Path

$version = (Select-String -Path "$RepoRoot\CMakeLists.txt" `
    -Pattern 'project\([^)]*VERSION\s+([\d.]+)').Matches[0].Groups[1].Value

$staging = Join-Path $RepoRoot 'staging'
$stagingBin = Join-Path $staging 'bin'
$exe = Join-Path $stagingBin 'wows-extractor.exe'
$msiOut = Join-Path $RepoRoot "wows-extractor-${version}-win64.msi"

if (-not $SkipBuild) {
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if (-not $cmake) { throw 'cmake not found on PATH' }
    Remove-Item -Recurse -Force $staging -ErrorAction SilentlyContinue
    $vcvars = Find-VcVars64
    if ($vcvars) {
        $installCmd = "`"$vcvars`" && cd /d `"$RepoRoot`" && `"$($cmake.Source)`" --install build --config $BuildType --prefix `"$staging`""
        cmd /c $installCmd
    } else {
        & $cmake.Source --install build --config $BuildType --prefix $staging
    }
    if ($LASTEXITCODE -ne 0) { throw "cmake --install failed (exit $LASTEXITCODE)" }
    if (-not (Test-Path $exe)) { throw "Missing $exe after install. Build the project first." }
}

if (-not (Test-Path $exe)) { throw "Missing $exe. Build and install, or run without -SkipBuild." }

Write-Host "==> windeployqt"
& windeployqt --release --no-translations --qmldir (Join-Path $RepoRoot 'src') $exe
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed (exit $LASTEXITCODE)" }

Write-Host "==> Bundle Python DLL"
$pyDll = Get-ChildItem $PythonDir -Filter 'python3*.dll' |
    Where-Object { $_.Name -match '^python\d+\.dll$' } |
    Select-Object -First 1
if (-not $pyDll) { throw "Python DLL not found in $PythonDir" }
Copy-Item $pyDll.FullName $stagingBin -Force
Write-Host "Bundled: $($pyDll.Name)"

Remove-Item -Force (Join-Path $RepoRoot 'AppFiles.wxs'),
    (Join-Path $RepoRoot 'AppFiles.wixobj'),
    (Join-Path $RepoRoot 'installer.wixobj'),
    $msiOut -ErrorAction SilentlyContinue

Write-Host "==> heat.exe"
& (Join-Path $WixDir 'heat.exe') dir $stagingBin -nologo -gg -suid `
    -dr INSTALLDIR -cg AppFiles -var var.SourceDir `
    -out (Join-Path $RepoRoot 'AppFiles.wxs')
if ($LASTEXITCODE -ne 0) { throw "heat.exe failed (exit $LASTEXITCODE)" }

$icon = Join-Path $RepoRoot 'packaging\windows\wows-extractor.ico'
Write-Host "==> candle.exe"
& (Join-Path $WixDir 'candle.exe') -nologo -arch x64 -ext WixUIExtension `
    "-dProductVersion=$version" `
    "-dSourceDir=$stagingBin" `
    "-dIconFile=$icon" `
    (Join-Path $RepoRoot 'AppFiles.wxs') `
    (Join-Path $RepoRoot 'packaging\windows\installer.wxs') `
    -out "$RepoRoot\"
if ($LASTEXITCODE -ne 0) { throw "candle.exe failed (exit $LASTEXITCODE)" }

Write-Host "==> light.exe"
& (Join-Path $WixDir 'light.exe') -nologo -ext WixUIExtension -cultures:en-us `
    (Join-Path $RepoRoot 'AppFiles.wixobj') `
    (Join-Path $RepoRoot 'installer.wixobj') `
    -out $msiOut
if ($LASTEXITCODE -ne 0) { throw "light.exe failed (exit $LASTEXITCODE)" }

Write-Host "MSI created: $msiOut"
Get-Item $msiOut | Format-List FullName, @{N='SizeMB';E={[math]::Round($_.Length/1MB,2)}}, LastWriteTime
