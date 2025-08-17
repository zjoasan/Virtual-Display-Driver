@echo off
echo Building IddCx Version Query Tool...
echo.

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    cl /EHsc IddCxVersionQuery.cpp /Fe:IddCxVersionQuery.exe
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    cl /EHsc IddCxVersionQuery.cpp /Fe:IddCxVersionQuery.exe
) else (
    echo Visual Studio not found. Trying g++...
    g++ -std=c++11 -o IddCxVersionQuery.exe IddCxVersionQuery.cpp
)

if exist IddCxVersionQuery.exe (
    echo.
    echo Build successful! Run IddCxVersionQuery.exe to check IddCx version.
) else (
    echo.
    echo Build failed. Please ensure you have a C++ compiler installed.
    echo You can install Visual Studio Community or MinGW-w64.
)

pause