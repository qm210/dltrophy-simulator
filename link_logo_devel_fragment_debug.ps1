$file = ".\cmake-build-debug\shaders\logoDevelFrag.glsl"
$backupFile = $file + ".bak"
# $dir = Split-Path $backupFile
$backupName = Split-Path $backupFile -Leaf

if (Test-Path $file) {
    if (Test-Path $backupFile) {
        Remove-Item $backupFile -Force
    }
    Rename-Item -Path $file -NewName $backupName -Force
}

New-Item -Path $file -ItemType SymbolicLink -Target ($PWD.Path + "\src\shaders\logoDevelFrag.glsl")
