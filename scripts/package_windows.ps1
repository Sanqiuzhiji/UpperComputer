param(
    [string]$QtRoot = "D:\Professional_Use_Software\Qt\6.11.1\msvc2022_64",
    [string]$BuildDirectory = "cmake-build-release-msvc-2022",
    [string]$OutputDirectory = "dist",
    [switch]$ExcludeWorkspaces
)

$ErrorActionPreference = "Stop"

$repositoryRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $PSScriptRoot ".."))
$buildPath = [System.IO.Path]::GetFullPath(
    (Join-Path $repositoryRoot $BuildDirectory))
$outputPath = [System.IO.Path]::GetFullPath(
    (Join-Path $repositoryRoot $OutputDirectory))
$repositoryPrefix = $repositoryRoot.TrimEnd('\') + '\'

foreach ($path in @($buildPath, $outputPath)) {
    if (-not $path.StartsWith(
            $repositoryPrefix,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Build and output directories must stay inside the repository: $path"
    }
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} `
    "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
    throw "Visual Studio Installer's vswhere.exe was not found."
}
$visualStudioRoot = (& $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath).Trim()
if (-not $visualStudioRoot) {
    throw "Visual Studio 2022 with the C++ x64 build tools was not found."
}
$vcvars = Join-Path $visualStudioRoot "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path -LiteralPath $vcvars -PathType Leaf)) {
    throw "vcvars64.bat was not found: $vcvars"
}

# Import the compiler environment so the script also works in a normal
# PowerShell window, not only in a Visual Studio developer prompt.
$compilerPath = $null
cmd.exe /d /s /c "`"$vcvars`" >nul && set" | ForEach-Object {
    $separator = $_.IndexOf('=')
    if ($separator -gt 0) {
        $name = $_.Substring(0, $separator)
        $value = $_.Substring($separator + 1)
        if ($name -ceq "PATH") {
            $compilerPath = $value
        } elseif ($name -ieq "PATH") {
            # cmd can expose both PATH (updated by vcvars) and the inherited
            # mixed-case Path. Keep the compiler-aware uppercase value.
            return
        } else {
            [Environment]::SetEnvironmentVariable(
                $name, $value, [EnvironmentVariableTarget]::Process)
        }
    }
}
if (-not $compilerPath) {
    throw "vcvars64.bat did not return an updated PATH."
}
# Some hosts expose both `Path` and `PATH`. MSBuild treats that as a duplicate
# key when it launches cl.exe, so normalize them to one process variable.
[Environment]::SetEnvironmentVariable(
    "Path", $null, [EnvironmentVariableTarget]::Process)
[Environment]::SetEnvironmentVariable(
    "PATH", $null, [EnvironmentVariableTarget]::Process)
$env:Path = $compilerPath
$env:QTFRAMEWORK_BYPASS_LICENSE_CHECK = "1"
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw "The MSVC compiler environment could not be loaded."
}

$cmake = Get-Command cmake -ErrorAction Stop
$deployQt = Join-Path $QtRoot "bin\windeployqt.exe"
if (-not (Test-Path -LiteralPath $deployQt -PathType Leaf)) {
    throw "windeployqt.exe was not found under QtRoot: $deployQt"
}

& $cmake.Source -S $repositoryRoot -B $buildPath `
    -G "Visual Studio 17 2022" -A x64 `
    "-DCMAKE_PREFIX_PATH=$QtRoot" `
    -DBUILD_TESTING=OFF `
    -DUPPERCOMPUTER_PORTABLE_BUILD=ON
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

& $cmake.Source --build $buildPath --config Release --target UpperComputer
if ($LASTEXITCODE -ne 0) { throw "Release build failed." }

$executable = Join-Path $buildPath "Release\UpperComputer.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Release executable was not produced: $executable"
}

$stagingPath = Join-Path $outputPath "UpperComputer"
$zipPath = Join-Path $outputPath "UpperComputer-0.1.0-win64.zip"

if (Test-Path -LiteralPath $stagingPath) {
    Remove-Item -LiteralPath $stagingPath -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
New-Item -ItemType Directory -Path $stagingPath -Force | Out-Null
Copy-Item -LiteralPath $executable -Destination $stagingPath

& $deployQt --release --compiler-runtime --no-translations `
    --dir $stagingPath (Join-Path $stagingPath "UpperComputer.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed." }

if (-not $ExcludeWorkspaces) {
    $workspaceSource = Join-Path $repositoryRoot "workspaces"
    if (Test-Path -LiteralPath $workspaceSource -PathType Container) {
        Copy-Item -LiteralPath $workspaceSource -Destination $stagingPath `
            -Recurse -Force
    }
}

New-Item -ItemType Directory `
    -Path (Join-Path $stagingPath "config") -Force | Out-Null
New-Item -ItemType Directory `
    -Path (Join-Path $stagingPath "workspaces\plot") -Force | Out-Null
New-Item -ItemType Directory `
    -Path (Join-Path $stagingPath "workspaces\protocols") -Force | Out-Null

Compress-Archive -LiteralPath $stagingPath -DestinationPath $zipPath `
    -CompressionLevel Optimal

$zip = Get-Item -LiteralPath $zipPath
Write-Host "Package created successfully:"
Write-Host "  Folder: $stagingPath"
Write-Host "  ZIP:    $($zip.FullName) ($([math]::Round($zip.Length / 1MB, 2)) MB)"
