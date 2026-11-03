@echo off
cl /std:c++17 /EHsc /O2 /W4 /I include src\main.cpp /Fe:losv_converter.exe
if errorlevel 1 exit /b 1
echo Build successful: losv_converter.exe
