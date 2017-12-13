@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64     
set compilerflags=/Od /Zi /EHsc
set compilerflags=/O3 /Zi /EHsc
set linkerflags=/OUT:arrays_test.exe
cl.exe %compilerflags% arrays_test.cpp /link %linkerflags%
