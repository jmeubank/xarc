cmake_minimum_required(VERSION 3.10)

project(xarc VERSION 0.1)

include_directories(
    "include"
)
#add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -fno-rtti")

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
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/7zAlloc.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/7zArcIn.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/7zBuf.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/7zCrc.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/7zCrcOpt.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/7zDec.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/7zFile.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/7zStream.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/Alloc.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/Bcj2.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/Bra.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/Bra86.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/BraIA64.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/CpuArch.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/Delta.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/Lzma2Dec.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/LzmaDec.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/MtDec.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/Sha256.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/Threads.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/Xz.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/XzCrc64.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/XzCrc64Opt.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C/XzDec.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{BZIP2_DIRNAME}/blocksort.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{BZIP2_DIRNAME}/bzlib.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{BZIP2_DIRNAME}/compress.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{BZIP2_DIRNAME}/crctable.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{BZIP2_DIRNAME}/decompress.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{BZIP2_DIRNAME}/huffman.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{BZIP2_DIRNAME}/randtable.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{ZLIB_DIRNAME}/adler32.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{ZLIB_DIRNAME}/crc32.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{ZLIB_DIRNAME}/gzlib.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{ZLIB_DIRNAME}/gzread.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{ZLIB_DIRNAME}/inffast.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{ZLIB_DIRNAME}/inflate.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{ZLIB_DIRNAME}/inftrees.c"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{ZLIB_DIRNAME}/zutil.c"
)
target_include_directories(
    xarc PRIVATE
    "src/libxarc"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{LZMA_DIRNAME}/C"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{ZLIB_DIRNAME}"
    "${CMAKE_BINARY_DIR}/extlibs/$ENV{BZIP2_DIRNAME}"
    "src/third-party/zlib"
)
target_compile_options(xarc PUBLIC -O2 -flto)

add_executable(xtest src/xtest/xtest.cpp)
target_compile_options(xtest PRIVATE -g)
target_link_options(xtest PRIVATE -g)
target_link_libraries(xtest PRIVATE xarc)
