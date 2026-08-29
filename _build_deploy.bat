@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
set "CMAKE=C:\Program Files\CMake\bin\cmake.exe"
set "NINJA=C:\Users\Macta\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe\ninja.exe"
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
cd /d "%ROOT%\out\build\x86-Release"
if errorlevel 1 exit /b 1
"%CMAKE%" -G Ninja -DCMAKE_MAKE_PROGRAM=%NINJA% -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x86-windows-static "%ROOT%"
if errorlevel 1 exit /b 1
"%NINJA%"
if errorlevel 1 exit /b 1
copy /Y "%ROOT%\out\MODS\test\d2tweaks.dll" "%ROOT%\..\..\Mods\暗黑破坏神II若水暗月R2.2.9\d2tweaks.dll"
if errorlevel 1 exit /b 1
echo DONE
