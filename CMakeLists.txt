cmake_minimum_required(VERSION 3.10)

project(xarc VERSION 0.1)

include(ExternalProject)

ExternalProject_Add(
    xz
    URL file://C:/Users/joeub/Downloads/xz-5.2.4-windows.7z
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
ExternalProject_Get_property(xz SOURCE_DIR)
set(XZ_SOURCE_DIR ${SOURCE_DIR})

include_directories(
    "include"
)
add_definitions(-DUNICODE -D_UNICODE)

add_library(
    xarc
    "src/libxarc/decomp_bz2/decomp_bz2.c"
    "src/libxarc/decomp_gzip/decomp_gzip.c"
    "src/libxarc/decomp_lzma/decomp_lzma.c"
    "src/libxarc/decomp_xz/decomp_xz.c"
    "src/libxarc/filesys/filesys_win32.c"
    "src/libxarc/mod_7z/mod_7z.c"
    "src/libxarc/mod_minizip/ioapi.c"
    "src/libxarc/mod_minizip/iowin32.c"
    "src/libxarc/mod_minizip/mod_minizip.c"
    "src/libxarc/mod_minizip/unzip.c"
    "src/libxarc/mod_untar/mod_untar.c"
    "src/libxarc/type_constants.c"
    "src/libxarc/type_extensions.c"
    "src/libxarc/xarc_base.c"
    "src/libxarc/xarc_decompress.c"
    "src/libxarc/xarc_impl_cxx.cpp"
    "src/libxarc/xarc_impl.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/7zAlloc.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/7zArcIn.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/7zBuf.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/7zCrc.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/7zCrcOpt.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/7zDec.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/7zFile.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/7zStream.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/Bcj2.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/Bra.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/Bra86.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/BraIA64.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/CpuArch.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/Delta.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/Lzma2Dec.c"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C/LzmaDec.c"
    "src/third-party/bzip2/bzlib.c"
    "src/third-party/bzip2/crctable.c"
    "src/third-party/bzip2/decompress.c"
    "src/third-party/bzip2/huffman.c"
    "src/third-party/bzip2/randtable.c"
    "src/third-party/zlib/adler32.c"
    "src/third-party/zlib/crc32.c"
    "src/third-party/zlib/gzlib.c"
    "src/third-party/zlib/gzread.c"
    "src/third-party/zlib/inffast.c"
    "src/third-party/zlib/inflate.c"
    "src/third-party/zlib/inftrees.c"
    "src/third-party/zlib/zutil.c"
)
target_include_directories(
    xarc PUBLIC
    "src/libxarc"
    "${CMAKE_BINARY_DIR}/extlibs/lzma/C"
    "src/third-party/bzip2"
    "src/third-party/zlib"
    "${XZ_SOURCE_DIR}/include"
)
target_compile_options(xarc PUBLIC "-m32")

add_executable(xtest src/xtest/xtest.cpp)
target_compile_options(xtest PUBLIC "-m32")
target_link_options(xtest PUBLIC "-m32")
target_link_directories(xtest PUBLIC "${XZ_SOURCE_DIR}/bin_i686-sse2")
target_link_libraries(xtest PUBLIC xarc liblzma.a)
add_dependencies(xtest xz)
