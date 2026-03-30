@echo off
if not exist build mkdir build

pushd build
set flags=/nologo /Z7 /W4 /WX /analyze  /DBASE_DEBUG
set atlas=atlas.exe
cl  %flags% /Fe:tetris.exe ../win32_main.c /DPLATFORM_GRAPHICS /DPLATFORM_SOUND /DTETRIS_ENABLE_CHEAT
cl  %flags% /Fe:%atlas% ../atlas_main.c 
%atlas%
popd build
