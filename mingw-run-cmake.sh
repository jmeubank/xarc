#!/usr/bin/sh

mkdir -p extlibs && \
  (
    [ -d extlibs/lzma ] || ( \
      mkdir -p extlibs/lzma && \
      cd extlibs/lzma && \
      curl -s -O -J -L file://C:/Users/joeub/Downloads/lzma1900.7z && \
      7z x -y lzma1900.7z \
    ) \
  ) && \
  cmake -G "MSYS Makefiles" $(dirname "$0") && \
  make
