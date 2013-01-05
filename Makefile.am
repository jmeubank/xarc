## Process this file with automake to produce Makefile.in

# Makefile for XARC
# Created: JohnE, 2010-09-11

noinst_PROGRAMS = xtest
noinst_LIBRARIES = libxarc.a lib7z_min.a

xtest_CFLAGS = -I$(srcdir)
xtest_LDADD = libxarc.a lib7z_min.a
xtest_SOURCES = test_linux.c

libxarc_a_CFLAGS = -I$(srcdir) -I$(srcdir)/7z
libxarc_a_SOURCES = \
 decomp_bz2/decomp_bz2.c \
 decomp_gzip/decomp_gzip.c \
 decomp_lzma/decomp_lzma.c \
 filesys/filesys_posix.c \
 mod_7z/mod_7z.c \
 mod_minizip/ioapi.c \
 mod_minizip/mod_minizip.c \
 mod_minizip/unzip.c \
 mod_untar/mod_untar.c \
 type_constants.c \
 type_extensions.c \
 xarc_base.c \
 xarc_decompress.c \
 xarc_impl.c

lib7z_min_a_SOURCES = \
 7z/Util/7z/7zAlloc.c \
 7z/7zBuf.c \
 7z/7zCrc.c \
 7z/7zCrcOpt.c \
 7z/7zDec.c \
 7z/7zFile.c \
 7z/7zIn.c \
 7z/7zStream.c \
 7z/Bcj2.c \
 7z/Bra86.c \
 7z/CpuArch.c \
 7z/Lzma2Dec.c \
 7z/LzmaDec.c
