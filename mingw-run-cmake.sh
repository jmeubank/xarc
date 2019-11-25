#!/usr/bin/sh

export LZMA_URL="file://C:/Users/joeub/Downloads/lzma1900.7z"
export LZMA_FILENAME="lzma1900.7z"
export LZMA_DIRNAME="lzma"
export ZLIB_URL="file://C:/Users/joeub/Downloads/zlib-1.2.11.tar.xz"
export ZLIB_DIRNAME="zlib-1.2.11"

mkdir -p extlibs && \
  ( \
    [ -d "extlibs/lzma" ] || ( \
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
