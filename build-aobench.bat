@rem if parameter 1 is -debug then the executable will contain debug information 

@cd "%~dp0"
@call toolchain\setpaths.bat
@call cleanup.bat

@set vbcc_asmfiles=source\startup.asm
@set vbcc_cfiles=source\aobench.c
@set vbcc_libs=-lm881

@if x%1 == x-debug goto debug
:release
@echo Assembling, Compiling, and Linking dh0\mydemo.exe
@vc -final -size -fpu=68881 -O2 -notmpfile -nostdlib -dontwarn=166 -dontwarn=307 -dontwarn=306 %vbcc_libs% -o dh0\mydemo.exe %vbcc_asmfiles% %vbcc_cfiles%
@goto postbuild
:debug
@echo Assembling, Compiling, and Linking dh0\mydemo.exe - with debug info
@vc -final -size -fpu=68881 -O2 -g -notmpfile -nostdlib -dontwarn=166 -dontwarn=307 -dontwarn=306 %vbcc_libs% -o dh0\mydemo.exe %vbcc_asmfiles% %vbcc_cfiles% 

:postbuild
@if exist dh0\mydemo.exe goto success

:error
@echo Build failed
@pause
@exit /b 1
:success
@echo Build success
@pause
@exit /b 0
