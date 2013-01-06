/* File: libxarc/modules.inc
 * The central registry for the modules XARC uses to handle individual archive
 * types.
 */

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
	XARC_HANDLE_2PHASE(XARC_TAR_LZMA, decomp_xz_lzma)
	XARC_HANDLE_2PHASE(XARC_TAR_XZ, decomp_xz)
XARC_MODULE_END()

XARC_MODULE_BEGIN(mod_7z)
	XARC_HANDLE_1PHASE(XARC_7Z)
XARC_MODULE_END()
