pushd
$rootDirectory = (Join-Path -Path $PSScriptRoot -ChildPath .. -Resolve)
cd $rootDirectory

if (-not (Test-Path build)) {
	mkdir build
}
if (-not (Test-Path build\win32)) {
	mkdir build\win32
}

$defines = @(
	"-D_DEBUG"
)
clang -Ofast -Wall -Wno-unused-function -Wno-writable-strings -Wno-switch -g -nostdlib -fno-exceptions $defines -mfma -mavx2 .\main.cpp .\win32\win32.cpp .\win32\win32_clib_stub.c -lOpengl32 -lGdi32 -lUser32 -lXinput -lkernel32 -mavx2 -o .\build\win32\RTOneWeekend.exe "-Wl,/ENTRY:AppMain,/SUBSYSTEM:WINDOWS,/opt:ref" -Xlinker "/STACK:0x100000,0x100000"
popd
