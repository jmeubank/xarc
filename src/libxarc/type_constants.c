/* File: libxarc/type_constants.c
 * Create constants based on the <xarc/types.inc> registry.
 *
 * These will be used to tie the modules to XARC's file opening and
 * autodetection.
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


#include <stdint.h>


#define XARC_TYPE_BEGIN(id, name) \
 const uint8_t name = id;
#define XARC_EXTENSION(ext)
#define XARC_TYPE_END()
#include <xarc/types.inc>
#undef XARC_TYPE_BEGIN
#undef XARC_EXTENSION
#undef XARC_TYPE_END
