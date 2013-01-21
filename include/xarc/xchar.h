/* File: xarc/xchar.h
 * Flexible character types for XARC.
 */

/* Copyright 2013 John Eubank.

   This file is part of XARC.

   XARC is free software: you can redistribute it and/or modify it under the
   terms of the GNU Lesser General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   XARC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
   more details.

   You should have received a copy of the GNU Lesser General Public License
   along with XARC.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef XCHAR_H_INC
#define XCHAR_H_INC

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XCONCAT2
#define INNER_CONCAT2(a, b) a ## b
#define XCONCAT2(a, b) INNER_CONCAT2(a, b)
#endif


#ifndef XARC_NATIVE_WCHAR
#if (defined(_WIN32) || defined(_WIN64)) \
 && (defined(UNICODE) || defined(_UNICODE))

#include <wchar.h>
#define XARC_NATIVE_WCHAR 1

#else

#define XARC_NATIVE_WCHAR 0

#endif
#endif


#if XARC_NATIVE_WCHAR

#define XC(c) XCONCAT2(L, c)

#define xmkdir _wmkdir
#define xstrlen wcslen
#define xstrncasecmp _wcsnicmp
#if __MSVCRT_VERSION__ >= 0x0700
#define xstrerror _wcserror
#endif
#define xvsnprintf _vsnwprintf
#define xstrcpy wcscpy
#define xchmod _wchmod
#define xfopen _wfopen
#define xopen _wopen

typedef wchar_t xchar;


#else


#define XC(c) c

#define xmkdir mkdir
#define xstrlen strlen
#define xstrncasecmp strncasecmp
#define xstrerror strerror
#define xvsnprintf vsnprintf
#define xstrcpy strcpy
#define xchmod chmod
#define xfopen fopen
#define xopen open

typedef char xchar;


#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // XCHAR_H_INC
