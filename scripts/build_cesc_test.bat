@echo off
call "D:\ProfessionallyUsed_Folders\VS 2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
cmake --build cmake-build-debug-msvs-2022 --config Debug -j 4
if errorlevel 1 exit /b %errorlevel%
ctest --test-dir cmake-build-debug-msvs-2022 -C Debug --output-on-failure
