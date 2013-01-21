# Copyright 2013 John Eubank

#    This file is part of XARC.
#
#    XARC is free software: you can redistribute it and/or modify it under the
#    terms of the GNU Lesser General Public License as published by the Free
#    Software Foundation, either version 3 of the License, or (at your option)
#    any later version.
#
#    XARC is distributed in the hope that it will be useful, but WITHOUT ANY
#    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
#    FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
#    more details.
#
#    You should have received a copy of the GNU Lesser General Public License
#    along with XARC.  If not, see <http://www.gnu.org/licenses/>.

noinst_PROGRAMS = xtest
noinst_LIBRARIES = libxarc.a lib7z_min.a

xtest_CFLAGS = -I$(srcdir)
xtest_LDADD = libxarc.a lib7z_min.a
xtest_SOURCES = test_linux.c

libxarc_a_CFLAGS = -I$(srcdir) -I$(srcdir)/7z
libxarc_a_SOURCES = \
 mod_untar/mod_untar.c \
 type_constants.c \
 type_extensions.c \
 xarc_base.c \
 xarc_decompress.c \
 xarc_impl.c

if HAVE_LIBZ
libxarc_a_SOURCES += \
 decomp_gzip/decomp_gzip.c \
 mod_minizip/ioapi.c \
 mod_minizip/mod_minizip.c \
 mod_minizip/unzip.c
endif

if HAVE_BZLIB
libxarc_a_SOURCES += decomp_bz2/decomp_bz2.c
endif

if HAVE_LIBLZMA
libxarc_a_SOURCES += decomp_xz/decomp_xz.c
endif

if HAVE_7Z
libxarc_a_SOURCES += \
 decomp_lzma/decomp_lzma.c \
 mod_7z/mod_7z.c
endif

if ON_WIN32_NATIVE
libxarc_a_SOURCES += filesys/filesys_win32.c
else
libxarc_a_SOURCES += filesys/filesys_posix.c
endif

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
