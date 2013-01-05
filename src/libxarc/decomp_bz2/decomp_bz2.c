/* File: decomp_bz2.c
 * Created: JohnE, 2010-08-08
 *
 * Implements BZIP2 decompression.
 */

#include "build.h"

#include <stdio.h>
#include <bzlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include "xarc_decompress.h"
#include "xarc_impl.h"
#include "filesys.h"


/* Struct: d_bz2_impl
 * Extends: <xarc_decompress_impl>
 *
 * Data specific to the BZIP2 decompression impl.
 */
typedef struct
{
	/* Variable: base
	 * The base <xarc_decompress_impl> object.
	 */
	xarc_decompress_impl base;
	/* Variable: infile
	 * The stdio stream for reading the archive file.
	 */
	FILE* infile;
	/* Variable: inbz2
	 * The BZIP2 decompression object.
	 */
	BZFILE* inbz2;
#if XARC_NATIVE_WCHAR
	/* Variable: localized_error
	 * Holds the localized return value of <d_bz2_error_desc> when the native
	 * character size is wider than a char.
	 */
	xchar* localized_error;
#endif
} d_bz2_impl;
#define D_BZ2(base) ((d_bz2_impl*)base)


/* Section: BZIP2 decompression wrappers
 * See also: <decomp_open_func>, <xarc_decompress_impl>
 */


xarc_result_t d_bz2_open(xarc* x, const xchar* path,
 xarc_decompress_impl** impl);
void d_bz2_close(xarc_decompress_impl* impl);
xarc_result_t d_bz2_read(xarc* x, xarc_decompress_impl* impl, void* buf,
 size_t* read_inout);
const xchar* d_bz2_error_desc(xarc_decompress_impl* impl, int32_t error_id);


/* Link d_bz2_open as the opener function for the decomp_bz2 module. */
XARC_DEFINE_DECOMPRESSOR(decomp_bz2, d_bz2_open)


/* Function: d_bz2_open
 *
 * Open a file for BZIP2 decompression.
 *
 * See also: <decomp_open_func>
 */
xarc_result_t d_bz2_open(xarc* x, const xchar* path, xarc_decompress_impl** impl)
{
	/* Open the file for reading */
	FILE* infile = xfopen(path, XC("rb"));
	if (!infile)
	{
		return xarc_set_error_filesys(x, XC("Failed to open '%s' for reading"),
		 path);
	}

	/* Open a BZIP2 decompression stream on the input file */
	int32_t bzerror;
	BZFILE* inbz2 = BZ2_bzReadOpen(&bzerror, infile, 0, 0, 0, 0);
	if (bzerror != BZ_OK)
	{
		xarc_set_error(x, XARC_DECOMPRESS_ERROR, bzerror,
		 XC("Failed to start BZIP2 stream on open file '%s'"), path);
		BZ2_bzReadClose(&bzerror, inbz2);
		fclose(infile);
		return XARC_DECOMPRESS_ERROR;
	}

	/* Allocate and fill out a d_bz2_impl object */
	d_bz2_impl* i = (d_bz2_impl*)malloc(sizeof(d_bz2_impl));
	i->base.close = d_bz2_close;
	i->base.read = d_bz2_read;
	i->base.error_desc = d_bz2_error_desc;
	i->infile = infile;
	i->inbz2 = inbz2;
#if XARC_NATIVE_WCHAR
	i->localized_error = 0;
#endif
	*impl = (xarc_decompress_impl*)i;

	return XARC_OK;
}


/* Function: d_bz2_close
 *
 * Close a previously opened BZIP2 file.
 *
 * See also: <xarc_decompress_impl>
 */
void d_bz2_close(xarc_decompress_impl* impl)
{
	/* Close the BZIP2 stream */
	int bzerror;
	BZ2_bzReadClose(&bzerror, D_BZ2(impl)->inbz2);
	/* Close the input file */
	fclose(D_BZ2(impl)->infile);
	/* Free heap memory */
#if XARC_NATIVE_WCHAR
	if (D_BZ2(impl)->localized_error)
		free(D_BZ2(impl)->localized_error);
#endif
	free(impl);
}


/* Function: d_bz2_read
 *
 * Read data from an opened BZIP2 file.
 *
 * See also: <xarc_decompress_impl>
 */
xarc_result_t d_bz2_read(xarc* x, xarc_decompress_impl* impl, void* buf,
 size_t* read_inout)
{
	int bzerror;
	int read = BZ2_bzRead(&bzerror, D_BZ2(impl)->inbz2, buf, *read_inout);

	/* For any result other than BZ_OK or BZ_STREAM_END, return due to an
	 * unrecoverable decompression error.
	 */
	if (bzerror != BZ_OK && bzerror != BZ_STREAM_END)
	{
		const char* edesc = BZ2_bzerror(D_BZ2(impl)->inbz2, &bzerror);
		if (bzerror == BZ_IO_ERROR)
		{
			return xarc_set_error_filesys(x,
			 XC("Error while reading BZIP2 data"));
		}
		else if (bzerror == BZ_DATA_ERROR_MAGIC)
		{
			return xarc_set_error(x, XARC_DECOMPRESS_ERROR, bzerror,
			 XC("Invalid BZIP2 stream"));
		}
		else
		{
#if XARC_NATIVE_WCHAR
			/* Localize the BZIP2 error string */
			size_t lenl = filesys_localize_char(edesc, -1, 0, 0);
			xchar* edescl = malloc(sizeof(xchar) * lenl);
			filesys_localize_char(edesc, -1, edescl, lenl);
			xarc_set_error(x, XARC_DECOMPRESS_ERROR, bzerror, edescl);
			free(edescl);
			return XARC_DECOMPRESS_ERROR;
#else
			return xarc_set_error(x, XARC_DECOMPRESS_ERROR, bzerror, edesc);
#endif
		}
	}

	/* Otherwise, if we read less than the amount requested, return
	 * XARC_DECOMPRESS_EOF.
	 */
	if ((size_t)read < *read_inout)
	{
		*read_inout = (size_t)read;
		return xarc_set_error(x, XARC_DECOMPRESS_EOF, 0,
		 XC("EOF while reading BZIP2 data"));
	}

	/* If we got here, everything is fine. */
	*read_inout = (size_t)read;
	return XARC_OK;
}


/* Function: d_bz2_error_desc
 *
 * Return a human-comprehensible string describing the most
 * recent BZIP2 library error.
 *
 * See also: <xarc_decompress_impl>
 */
const xchar* d_bz2_error_desc(xarc_decompress_impl* impl,
 int32_t error_id __attribute__((unused)))
{
	int errnum;
	const char* edesc = BZ2_bzerror(D_BZ2(impl)->inbz2, &errnum);
	/* BZ_IO_ERROR in errnum indicates an error in the standard I/O library; 
	 * anything else indicates a BZIP2-specific error.
	 */
	if (errnum == BZ_IO_ERROR)
		edesc = strerror(errno);

#if XARC_NATIVE_WCHAR
	/* Localize error string */
	size_t lenl = filesys_localize_char(edesc, -1, 0, 0);
	D_BZ2(impl)->localized_error = realloc(D_BZ2(impl)->localized_error,
	 sizeof(xchar) * lenl);
	filesys_localize_char(edesc, -1, D_BZ2(impl)->localized_error, lenl);
	return D_BZ2(impl)->localized_error;
#else
	return edesc;
#endif
}
