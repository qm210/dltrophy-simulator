[CmdletBinding(PositionalBinding = $false)]
param(
	[ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
	[string]$Config = "ReleaseWithDebInfo"
)

$OutDir = "build-$Config"
if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir | Out-Null
}

if (-not $Env:CC)  {
	$Env:CC  = "gcc.exe"
}
if (-not $Env:CXX) {
	$Env:CXX = "g++.exe"
}

cmake -S . -B $OutDir -G Ninja "-DCMAKE_BUILD_TYPE=$Config" "-DLINK_STATIC=OFF"

if ($LASTEXITCODE -ne 0) {
    $code = $LASTEXITCODE
    Write-Warning "CMake configuration failed (exit code $code). Check the error above; PATH may be missing CMake, Ninja, or MinGW."
    Write-Host "PATH=$Env:PATH"
    exit $code
}

cmake --build $OutDir --config $Config
