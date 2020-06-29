@echo off
setlocal

set "WD=%__CD__%"

REM the PATH environment needs to include 7z.exe, mingw32-make.exe and a working MinGW or MinGW-w64 GCC
REM installation.
set "PATH=C:\Program Files\7-Zip;C:\TDM-GCC-32\bin;%PATH%"

set "CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v3.17.3/cmake-3.17.3-win32-x86.zip"
set "CMAKE_FILENAME=cmake-3.17.3-win32-x86.zip"
set "CMAKE_DIRNAME=cmake-3.17.3-win32-x86"
set "BZIP2_URL=https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz"
set "BZIP2_FILENAME=bzip2-1.0.8.tar.gz"
set "BZIP2_DIRNAME=bzip2-1.0.8"
set "LZMA_URL=https://sourceforge.net/projects/sevenzip/files/LZMA%%20SDK/lzma1900.7z/download"
set "LZMA_FILENAME=lzma1900.7z"
set "LZMA_DIRNAME=lzma"
set "ZLIB_URL=https://github.com/madler/zlib/archive/v1.2.11.tar.gz"
set "ZLIB_FILENAME=zlib-1.2.11.tar.gz"
set "ZLIB_DIRNAME=zlib-1.2.11"

set "PATH=%WD%extlibs\%CMAKE_DIRNAME%\bin;%PATH%"

if not exist "%WD%extlibs/" ( mkdir "%WD%extlibs" || exit /b %ERRORLEVEL% )
if not exist "%WD%extlibs/%CMAKE_DIRNAME%/" (
    echo Downloading and extracting CMAKE
    pushd "%WD%/extlibs"
    curl -#JL "%CMAKE_URL%" -o "%CMAKE_FILENAME%" || exit /b %ERRORLEVEL%
    7z.exe x -y "%CMAKE_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%BZIP2_DIRNAME%/" (
    echo Downloading and extracting BZIP2
    pushd "%WD%/extlibs"
    curl -#JL "%BZIP2_URL%" -o "%BZIP2_FILENAME%" || exit /b %ERRORLEVEL%
    ( 7z.exe x -so -y "%BZIP2_FILENAME%" | 7z.exe x -si -y -ttar ) || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%LZMA_DIRNAME%/C" (
    echo Downloading and extracting LZMA SDK
    if not exist "%WD%extlibs/%LZMA_DIRNAME%/" ( mkdir "%WD%extlibs/%LZMA_DIRNAME%" || exit /b %ERRORLEVEL% )
    pushd "%WD%extlibs/%LZMA_DIRNAME%"
    curl -#JL "%LZMA_URL%" -o "%LZMA_FILENAME%" || exit /b %ERRORLEVEL%
    7z.exe x -y "%LZMA_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%ZLIB_DIRNAME%" (
    echo Downloading and extracting ZLIB
    pushd "%WD%extlibs"
    curl -#JL "%ZLIB_URL%" -o "%ZLIB_FILENAME%" || exit /b %ERRORLEVEL%
    ( 7z.exe x -so -y "%ZLIB_FILENAME%" | 7z.exe x -si -y -ttar ) || exit /b %ERRORLEVEL%
    popd
)

if not exist "%WD%Makefile" ( cmake.exe -G "MinGW Makefiles" "%~dp0" || exit /b %ERRORLEVEL% )

echo Going multithreaded
set VERBOSE=1
mingw32-make.exe -j12  || exit /b %ERRORLEVEL%

exit /b 0
