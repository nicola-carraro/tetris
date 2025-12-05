@echo off
if not exist build mkdir build

pushd build
set flags=/nologo /Z7 /W4 /WX  /DTTS_DEBUG
cl  %flags% /Fe:tetris.exe ../win32_main.c
cl  %flags% ../atlas.c
atlas.exe
popd build
