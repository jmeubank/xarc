/* File: xarc/types.inc
 * The central directory for the types of archives XARC recognizes.
 */

/* Macros: XARC archive types
 * A list of all the archive types supported by XARC.
 *
 * (0) "Autodetect" - Determine the type of archive from the file extension
 * (1) XARC_ZIP - A "zip archive", using GZIP compression (.zip)
 * (2) XARC_TAR_GZ - A "GZIP tarball", archived with TAR and using GZIP
 *   compression (.tar.gz, .tgz)
 * (3) XARC_TAR_BZ2 - A "BZIP2 tarball", archived with TAR and using BZIP2
 *   compression (.tar.bz2, .tbz)
 * (4) XARC_TAR_LZMA - An "LZMA tarball", archived with TAR and using LZMA
 *   compression (.tar.lzma, .tlz)
 * (5) XARC_TAR_LZMA - A "7-Zip archive", using LZMA compression and optional
 *   filters (.7z)
 * (6) XARC_TAR_XZ - An "XZ tarball", archived with TAR and using LZMA
 *   compression and optional filters (.tar.xz, .txz)
 */

XARC_TYPE_BEGIN(1, XARC_ZIP)
	XARC_EXTENSION("zip")
XARC_TYPE_END()

XARC_TYPE_BEGIN(2, XARC_TAR_GZ)
	XARC_EXTENSION("tar.gz")
	XARC_EXTENSION("tgz")
XARC_TYPE_END()

XARC_TYPE_BEGIN(3, XARC_TAR_BZ2)
	XARC_EXTENSION("tar.bz2")
	XARC_EXTENSION("tbz")
XARC_TYPE_END()

XARC_TYPE_BEGIN(4, XARC_TAR_LZMA)
	XARC_EXTENSION("tar.lzma")
	XARC_EXTENSION("tlz")
XARC_TYPE_END()

XARC_TYPE_BEGIN(5, XARC_7Z)
	XARC_EXTENSION("7z")
XARC_TYPE_END()

XARC_TYPE_BEGIN(6, XARC_TAR_XZ)
	XARC_EXTENSION("tar.xz")
	XARC_EXTENSION("txz")
XARC_TYPE_END()
