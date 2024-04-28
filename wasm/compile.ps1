pushd
$rootDirectory = (Join-Path -Path $PSScriptRoot -ChildPath .. -Resolve)
cd $rootDirectory

if (-not (Test-Path build)) {
	mkdir build
}
if (-not (Test-Path build\wasm)) {
	mkdir build\wasm
}

$defines = @(
	"-D_DEBUG",
	"-DCPU_WASM"
)
# clang -Wall -Wno-writable-strings -Wno-switch -g -nostdlib -fno-exceptions $defines -mfma -mavx2 .\main.cpp .\win32\win32.cpp .\win32\win32_clib_stub.c -lUser32 -lXinput -lkernel32 -mavx2 -o .\build\win32\RTOneWeekend.exe "-Wl,/ENTRY:_AppMain,/SUBSYSTEM:WINDOWS,/opt:ref" -Xlinker "/STACK:0x100000,0x100000"
clang -g -O3 $defines -Wall -Wno-unused-function -unused-function -nostdlib --target=wasm32 -msimd128 -mbulk-memory -Wall main.cpp .\wasm\wasm.cpp -Xlinker "--reproduce=build\wasm\main.wasm.map" -Xlinker "--no-entry" -o build\wasm\main.wasm
cp -Force wasm\*html build\wasm
cp -Force wasm\*js build\wasm
popd
