@echo off
 g++ -std=c++17 -O2 -Wall -Wextra -pedantic -Iinclude src\main.cpp -o losv_converter.exe
 if errorlevel 1 exit /b 1
 echo Build successful: losv_converter.exe
