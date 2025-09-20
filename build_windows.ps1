$Config = "Debug"
# $Config = "RelWithDebInfo"
# $Config = "Release"
# $Config = "MinSizeRel"
$OutDir = "build-$Config"

if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir | Out-Null
}

$Env:PATH = "D:\dev\mingw64\bin;$Env:PATH"
$ENV:CC = "gcc.exe"
$ENV:CXX = "g++.exe"

cmake -S . -B $OutDir -G Ninja "-DCMAKE_BUILD_TYPE=$Config" "-DLINK_STATIC=OFF"
cmake --build $OutDir --config $Config
