[CmdletBinding()]
param(
    [string]$NdkPath = "$env:LOCALAPPDATA\CodexTools\android-ndk\android-ndk-r27d",
    [string]$BuildRoot = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $BuildRoot) { $BuildRoot = Join-Path $projectRoot 'build' }

$cmake = "$env:LOCALAPPDATA\CodexTools\android-build-tools\cmake\data\bin\cmake.exe"
$ninja = "$env:LOCALAPPDATA\CodexTools\android-build-tools\bin\ninja.exe"
$toolchain = Join-Path $NdkPath 'build\cmake\android.toolchain.cmake'
if (-not (Test-Path -LiteralPath $cmake)) {
    $cmake = (Get-Command cmake -ErrorAction Stop).Source
}
if (-not (Test-Path -LiteralPath $ninja)) {
    $ninja = (Get-Command ninja -ErrorAction Stop).Source
}

foreach ($required in @($cmake, $ninja, $toolchain)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required build tool not found: $required"
    }
}

$abiRoot = Join-Path $BuildRoot 'arm64-v8a'
& $cmake -G Ninja `
    "-DCMAKE_MAKE_PROGRAM=$ninja" `
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain" `
    -DANDROID_ABI=arm64-v8a `
    -DANDROID_PLATFORM=android-28 `
    -DCMAKE_BUILD_TYPE=Release `
    -S (Join-Path $projectRoot 'cpp') `
    -B $abiRoot
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }

& $cmake --build $abiRoot
if ($LASTEXITCODE -ne 0) { throw 'Native build failed.' }

$zygiskRoot = Join-Path $BuildRoot 'zygisk'
New-Item -ItemType Directory -Path $zygiskRoot -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $abiRoot 'libpikmin_gps_copy.so') `
    -Destination (Join-Path $zygiskRoot 'arm64-v8a.so') -Force

Get-FileHash -LiteralPath (Join-Path $zygiskRoot 'arm64-v8a.so') -Algorithm SHA256
