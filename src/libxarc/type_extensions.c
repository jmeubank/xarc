/* File: libxarc/type_extensions.c
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


#include <xarc/xchar.h>


#ifndef XCONCAT2
#define INNER_CONCAT2(a, b) a ## b
#define XCONCAT2(a, b) INNER_CONCAT2(a, b)
#endif


#define XARC_TYPE_BEGIN(id, name) \
 const xchar* const XCONCAT2(xarc_type_extensions_, name)[] = {
#define XARC_EXTENSION(ext) \
 XC(ext),
#define XARC_TYPE_END() \
 0 };
#include <xarc/types.inc>
#undef XARC_TYPE_BEGIN
#undef XARC_EXTENSION
#undef XARC_TYPE_END
