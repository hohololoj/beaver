@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cl /nologo /W4 /O2 /Fe:mft.exe mft.c strhasiw.c