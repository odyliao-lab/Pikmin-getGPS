[CmdletBinding()]
param(
    [string]$BuildRoot = '',
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $BuildRoot) { $BuildRoot = Join-Path $projectRoot 'build' }
if (-not $OutputPath) {
    $OutputPath = Join-Path $projectRoot 'dist\pikmin-gps-copy-v152-r12.zip'
}

$templateRoot = Join-Path $projectRoot 'template\magisk_module'
$binary = Join-Path $BuildRoot 'zygisk\arm64-v8a.so'
$stageRoot = Join-Path $BuildRoot 'package'

foreach ($required in @($templateRoot, $binary)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required package input not found: $required"
    }
}

if (Test-Path -LiteralPath $stageRoot) {
    $resolvedBuild = (Resolve-Path -LiteralPath $BuildRoot).Path
    $resolvedStage = (Resolve-Path -LiteralPath $stageRoot).Path
    if (-not $resolvedStage.StartsWith(
            $resolvedBuild + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Package staging path escaped build root: $resolvedStage"
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}

New-Item -ItemType Directory -Path $stageRoot | Out-Null
Copy-Item -Path (Join-Path $templateRoot '*') -Destination $stageRoot -Recurse -Force
New-Item -ItemType Directory -Path (Join-Path $stageRoot 'zygisk') -Force | Out-Null
Copy-Item -LiteralPath $binary `
    -Destination (Join-Path $stageRoot 'zygisk\arm64-v8a.so') -Force

$moduleProperty = @(
    'id=zygisk_pikmin_gps_copy'
    'name=Pikmin GPS Copy'
    'version=v152.0-r12'
    'versionCode=152012'
    'author=odyliao-lab'
    'description=Copy the selected Pikmin Bloom expedition GPS to Android clipboard'
) -join "`n"
[IO.File]::WriteAllText(
    (Join-Path $stageRoot 'module.prop'),
    $moduleProperty + "`n",
    [Text.UTF8Encoding]::new($false))

$resolvedOutput = [IO.Path]::GetFullPath($OutputPath)
$outputDirectory = Split-Path -Parent $resolvedOutput
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
& tar.exe -a -cf $resolvedOutput -C $stageRoot `
    META-INF module.prop zygisk
if ($LASTEXITCODE -ne 0) { throw "Failed to create archive: $resolvedOutput" }

$entries = @(& tar.exe -tf $resolvedOutput)
if ($LASTEXITCODE -ne 0 -or
    $entries -notcontains 'META-INF/com/google/android/update-binary' -or
    $entries -notcontains 'zygisk/arm64-v8a.so' -or
    $entries -contains 'service.sh' -or
    $entries -contains 'customize.sh' -or
    $entries -match '\\') {
    throw "Invalid Magisk archive layout: $resolvedOutput"
}

Get-FileHash -LiteralPath $resolvedOutput -Algorithm SHA256
