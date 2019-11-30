@echo off
setlocal

set "WD=%__CD__%"

set "PATH=C:\TDM-GCC-32\bin;%PATH%"

set "SZ_EXE=C:/Program Files/7-Zip/7z.exe"
set "BZIP2_URL=file://C:/Users/joeub/Downloads/bzip2-1.0.8.tar.gz"
set "BZIP2_FILENAME=bzip2-1.0.8.tar.gz"
set "BZIP2_DIRNAME=bzip2-1.0.8"
set "LZMA_URL=file://C:/Users/joeub/Downloads/lzma1900.7z"
set "LZMA_FILENAME=lzma1900.7z"
set "LZMA_DIRNAME=lzma"
set "ZLIB_URL=file://C:/Users/joeub/Downloads/zlib-1.2.11.tar.xz"
set "ZLIB_FILENAME=zlib-1.2.11.tar.xz"
set "ZLIB_DIRNAME=zlib-1.2.11"

if not exist "%WD%extlibs/" ( mkdir "%WD%extlibs" || exit /b %ERRORLEVEL% )
if not exist "%WD%extlibs/%BZIP2_DIRNAME%/" (
    echo Downloading and extracting BZIP2
    pushd "%WD%/extlibs"
    curl -#JLO "%BZIP2_URL%" || exit /b %ERRORLEVEL%
    ( "%SZ_EXE%" x -so -y "%BZIP2_FILENAME%" | "%SZ_EXE%" x -si -y -ttar ) || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%LZMA_DIRNAME%/C" (
    echo Downloading and extracting LZMA SDK
    if not exist "%WD%extlibs/%LZMA_DIRNAME%/" ( mkdir "%WD%extlibs/%LZMA_DIRNAME%" || exit /b %ERRORLEVEL% )
    pushd "%WD%extlibs/%LZMA_DIRNAME%"
    curl -#JLO "%LZMA_URL%" || exit /b %ERRORLEVEL%
    "%SZ_EXE%" x -y "%LZMA_FILENAME%" || exit /b %ERRORLEVEL%
    popd
)
if not exist "%WD%extlibs/%ZLIB_DIRNAME%" (
    echo Downloading and extracting ZLIB
    pushd "%WD%extlibs"
    curl -#JLO "%ZLIB_URL%" || exit /b %ERRORLEVEL%
    ( "%SZ_EXE%" x -so -y "%ZLIB_FILENAME%" | "%SZ_EXE%" x -si -y -ttar ) || exit /b %ERRORLEVEL%
    popd
)

if not exist "%WD%Makefile" ( cmake.exe -G "MinGW Makefiles" "%~dp0" || exit /b %ERRORLEVEL% )

echo Going multithreaded
set VERBOSE=1
mingw32-make.exe -j12  || exit /b %ERRORLEVEL%

exit /b 0
