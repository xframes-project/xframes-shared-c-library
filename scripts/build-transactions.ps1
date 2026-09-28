param(
    [string]$XFramesSource = (Join-Path $PSScriptRoot '../xframes'),
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '../build/transactions'),
    [string]$VcpkgInstalled
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourceRoot = (Resolve-Path -LiteralPath $XFramesSource).Path
$buildRoot = [IO.Path]::GetFullPath($BuildDirectory)
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'Visual Studio C++ tools were not found.' }
& (Join-Path $installation 'Common7/Tools/Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
$cmake = Join-Path $installation 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ninja = Join-Path $installation 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'
$toolchain = Join-Path $sourceRoot 'packages/dear-imgui/cpp/deps/vcpkg/scripts/buildsystems/vcpkg.cmake'
$configure = @('-S', $repo, '-B', $buildRoot, '-G', 'Ninja', '-DXFRAMES_TRANSACTIONAL=ON', '-DCMAKE_BUILD_TYPE=Release',
    "-DCMAKE_MAKE_PROGRAM=$ninja", "-DXFRAMES_SOURCE_DIR=$sourceRoot", "-DCMAKE_TOOLCHAIN_FILE=$toolchain")
if ($VcpkgInstalled) {
    $configure += @('-DVCPKG_MANIFEST_INSTALL=OFF', "-DVCPKG_INSTALLED_DIR=$([IO.Path]::GetFullPath($VcpkgInstalled))")
}
& $cmake @configure
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& $cmake --build $buildRoot --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Native build failed.' }
Write-Output "Built $(Join-Path $buildRoot 'xframesshared.dll')"
