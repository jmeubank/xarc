#!/usr/bin/sh

export BZIP2_URL="file://C:/Users/joeub/Downloads/bzip2-1.0.8.tar.gz"
export BZIP2_DIRNAME="bzip2-1.0.8"
export LZMA_URL="file://C:/Users/joeub/Downloads/lzma1900.7z"
export LZMA_FILENAME="lzma1900.7z"
export LZMA_DIRNAME="lzma"
export ZLIB_URL="file://C:/Users/joeub/Downloads/zlib-1.2.11.tar.xz"
export ZLIB_DIRNAME="zlib-1.2.11"

mkdir -p extlibs && \
  ( \
    [ -d "extlibs/$BZIP2_DIRNAME" ] || ( \
      mkdir -p extlibs && \
      cd extlibs && \
      curl -#L -o- "$BZIP2_URL" | bsdtar -xf - \
    ) \
  ) && \
  ( \
    [ -d "extlibs/lzma/C" ] || ( \
      mkdir -p extlibs/lzma && \
      cd extlibs/lzma && \
      curl -#JL -O "$LZMA_URL" && \
      7z x $LZMA_FILENAME \
    ) \
  ) && \
  ( \
    [ -d "extlibs/$ZLIB_DIRNAME" ] || ( \
      mkdir -p extlibs && \
      cd extlibs && \
      curl -#L -o- "$ZLIB_URL" | bsdtar -xf - \
    ) \
  ) && \
  cmake -G "MSYS Makefiles" $(dirname "$0") && \
  make
