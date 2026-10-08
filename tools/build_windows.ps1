param(
    [Parameter(Mandatory=$true)][string]$DusklightSource,
    [Parameter(Mandatory=$true)][string]$ImportLibrary,
    [string]$BuildDirectory = ''
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Get-ExistingPath([string]$Path, [bool]$Directory) {
    $item = Get-Item -LiteralPath $Path -ErrorAction Stop
    if ($item.PSIsContainer -ne $Directory) { throw "Unexpected path type: $Path" }
    return $item.FullName
}

$projectRoot = (Get-Item -LiteralPath (Split-Path $PSScriptRoot -Parent)).FullName
$sourceRoot = Get-ExistingPath $DusklightSource $true
$importPath = Get-ExistingPath $ImportLibrary $false
$compiler = Get-Command cl.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
if (!$compiler) { throw 'Run this script from an x64 Visual Studio developer shell with cl.exe on PATH.' }
if ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64') { throw 'This mod requires the x64 Visual Studio developer environment.' }
if (!$env:INCLUDE -or !$env:LIB) { throw 'The MSVC INCLUDE and LIB environment variables must be configured.' }
$python = Get-Command python.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
$pythonArguments = @()
if (!$python) {
    $python = Get-Command py.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    $pythonArguments = @('-3')
}
if (!$python) { throw 'Python 3 is required to package the mod.' }
& $python.Source @pythonArguments -c 'import sys; sys.exit(0 if sys.version_info >= (3, 8) else 1)'
if ($LASTEXITCODE -ne 0) { throw 'Python 3.8 or later is required to package the mod.' }

$includeDirectories = @('include','assets\GZ2E01','libs\JSystem\include','extern\aurora\include\dolphin','extern\aurora\include','sdk\include','src')
foreach ($include in $includeDirectories) { [void](Get-ExistingPath (Join-Path $sourceRoot $include) $true) }
$modSource = Get-ExistingPath (Join-Path $projectRoot 'src\mod.cpp') $false
$gameFeature = Get-ExistingPath (Join-Path $sourceRoot 'sdk\src\game_feature.cpp') $false
[void](Get-ExistingPath (Join-Path $sourceRoot 'include\global.h') $false)
[void](Get-ExistingPath (Join-Path $projectRoot 'mod.json') $false)
[void](Get-ExistingPath (Join-Path $projectRoot 'res') $true)
if (!$BuildDirectory) { $BuildDirectory = Join-Path $projectRoot 'build' }
$buildRoot = [IO.Path]::GetFullPath($BuildDirectory)
foreach ($protectedPath in @($sourceRoot,$projectRoot,(Join-Path $projectRoot 'src'),(Join-Path $projectRoot 'res'),$PSScriptRoot)) {
    if ($buildRoot.Equals($protectedPath,[StringComparison]::OrdinalIgnoreCase)) { throw 'BuildDirectory must be a separate build folder, not a source/resource folder.' }
}
foreach ($protectedPath in @((Join-Path $projectRoot 'src'),(Join-Path $projectRoot 'res'),$PSScriptRoot)) {
    if ($buildRoot.StartsWith($protectedPath + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'BuildDirectory cannot be inside src, res, or tools.' }
}
$objectRoot = Join-Path $buildRoot 'obj'
$libraryRoot = Join-Path $buildRoot 'lib\windows-amd64'
$bundleRoot = Join-Path $buildRoot 'mods'
foreach ($directory in @($buildRoot,$objectRoot,$libraryRoot,$bundleRoot)) { New-Item -ItemType Directory -Path $directory -Force | Out-Null }
$libraryPath = Join-Path $libraryRoot 'mod.dll'
$bundlePath = Join-Path $bundleRoot 'kingdom_key.dusk'

# Tested release settings for the Dusklight 2.0.3 Windows ABI. The caller
# supplies the compiler, Windows SDK, INCLUDE and LIB environment.
$compileArguments = @('/nologo','/LD','/MD','/std:c++20','/Zc:__cplusplus','/EHsc','/O2','/bigobj','/utf-8','/FIglobal.h','/DTARGET_PC=1','/DWIDESCREEN_SUPPORT=1','/DAVOID_UB=1','/DVERSION=0','/DMTX_USE_PS=1','/DPARTIAL_DEBUG=1','/DDUSK_MOD_FEATURE_GAME=1')
foreach ($include in $includeDirectories) { $compileArguments += ('/I' + (Join-Path $sourceRoot $include)) }
$compileArguments += @($modSource,$gameFeature,('/Fo' + $objectRoot + '\'),'/link',('/OUT:' + $libraryPath),('/IMPLIB:' + (Join-Path $libraryRoot 'mod.lib')),$importPath)
Push-Location -LiteralPath $buildRoot
try {
    & $compiler.Source @compileArguments
    if ($LASTEXITCODE -ne 0) { throw "MSVC compilation/linking failed with exit code $LASTEXITCODE." }
    & $python.Source @pythonArguments (Join-Path $PSScriptRoot 'package_mod.py') --project $projectRoot --dll $libraryPath --output $bundlePath
    if ($LASTEXITCODE -ne 0) { throw "Packaging failed with exit code $LASTEXITCODE." }
} finally {
    Pop-Location
}
Write-Host "Built bundle: $bundlePath"
