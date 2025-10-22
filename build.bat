@echo off
set BUILD_DIR=out\build
set BIN_DIR=%BUILD_DIR%\bin\Release

echo === Configurando com CMake ===
cmake -S . -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b %errorlevel%

echo === Compilando ===
cmake --build %BUILD_DIR% --config Release
if errorlevel 1 exit /b %errorlevel%

cd %BIN_DIR%

