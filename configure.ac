#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([xarc], [0.1])
AC_CONFIG_SRCDIR([src/libxarc/xarc_base.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])

AC_ARG_WITH([zlib], [
  AS_HELP_STRING([--with-zlib[[=PATH]]], [
    use system or PATH-prefixed zlib [default: use system zlib if available]
  ])
], [], [with_zlib=check])

AC_ARG_WITH([bzlib], [
  AS_HELP_STRING([--with-bzlib[[=PATH]]], [
    use system or PATH-prefixed bzlib [default: use system bzlib if available]
  ])
], [], [with_bzlib=check])

AC_ARG_WITH([7z-src], [
  AS_HELP_STRING([--with-7z-src=PATH], [
    compile in LZMA and 7Z support from 7-Zip LZMA SDK sources at PATH [default:
    do not compile in LZMA or 7Z support]
  ])
], [], [with_7z_src=no])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# ZLib
AS_IF([test "x$with_zlib" != xno], [
  if test "x$with_zlib" != xcheck && test "x$with_zlib" != xyes; then
    LDFLAGS="-L$with_zlib/lib $LDFLAGS"
  fi
  AC_CHECK_LIB([z], [gzopen], [
    AC_CHECK_HEADER([zlib.h], [
      AC_DEFINE([HAVE_LIBZ], [1], [Define if you have libz])
    ], [
      if test "x$with_zlib" != xcheck; then
        AC_MSG_FAILURE([--with-zlib specified but zlib.h not found])
      fi
    ])
  ], [
    if test "x$with_zlib" != xcheck; then
      AC_MSG_FAILURE([--with-zlib specified but libz not found])
    fi
  ])
])

# BZIP2
AS_IF([test "x$with_bzlib" != xno], [
  if test "x$with_bzlib" != xcheck && test "x$with_bzlib" != xyes; then
    LDFLAGS="-L$with_bzlib/lib $LDFLAGS"
  fi
  AC_CHECK_LIB([bz2], [BZ2_bzReadOpen], [
    AC_CHECK_HEADER([bzlib.h], [
      AC_DEFINE([HAVE_BZLIB], [1], [Define if you have libbz2])
    ], [
      if test "x$with_bzlib" != xcheck; then
        AC_MSG_FAILURE([--with-bzlib specified but bzlib.h not found])
      fi
    ])
  ], [
    if test "x$with_bzlib" != xcheck; then
      AC_MSG_FAILURE([--with-bzlib specified but libbz2 not found])
    fi
  ])
])

# 7-Zip SDK
AS_IF([test "x$with_7z_src" != xno], [
  CPPFLAGS="-I$with_7z_src/C $CPPFLAGS"
  CFLAGS="-I$with_7z_src/C $CFLAGS"
  AC_CHECK_HEADER([7z.h], [
    AC_DEFINE([HAVE_7Z_SDK], [1], [Define if you have the 7-Zip LZMA SDK])
  ], [
    AC_MSG_FAILURE([--with-7z-src specified but 7z.h not found])
  ])
])

# Checks for libraries.
AC_DEFUN([MINGW_AC_WIN32_NATIVE_HOST],
[AC_CACHE_CHECK([whether we are building for a Win32 host], [mingw_cv_win32_host],
 AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#ifdef _WIN32
 choke me
#endif]])], [mingw_cv_win32_host=no], [mingw_cv_win32_host=yes]))dnl
])# MINGW_AC_WIN32_NATIVE_HOST
MINGW_AC_WIN32_NATIVE_HOST

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
