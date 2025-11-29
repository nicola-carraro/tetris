@echo off
if not exist build mkdir build

pushd build
cl /nologo /Z7 /W4 /WX /Fe:tetris.exe ../win32_main.c
popd build