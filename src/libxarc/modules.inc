/* File: libxarc/modules.inc
 * The central registry for the modules XARC uses to handle individual archive
 * types.
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

/* Macros: XARC module list
 * A list of all the archive modules included in XARC.
 *
 * mod_minizip - Handles ZIP archives.
 * mod_untar - Handles any type of compressed tarball, using <XARC
 *   decompressors>.
 * mod_7z - Handles 7-Zip archives.
 */

XARC_MODULE_BEGIN(mod_minizip)
	XARC_HANDLE_1PHASE(XARC_ZIP)
XARC_MODULE_END()

XARC_MODULE_BEGIN(mod_untar)
	XARC_HANDLE_2PHASE(XARC_TAR_GZ, decomp_gzip)
	XARC_HANDLE_2PHASE(XARC_TAR_BZ2, decomp_bz2)
	XARC_HANDLE_2PHASE(XARC_TAR_LZMA, decomp_lzma)
	XARC_HANDLE_2PHASE(XARC_TAR_XZ, decomp_xz)
XARC_MODULE_END()

XARC_MODULE_BEGIN(mod_7z)
	XARC_HANDLE_1PHASE(XARC_7Z)
XARC_MODULE_END()
