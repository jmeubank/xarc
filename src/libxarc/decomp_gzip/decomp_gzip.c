/* File: decomp_gzip.c
 * Created: JohnE, 2010-08-05
 *
 * Implements GZIP decompression.
 */

#include "build.h"

#include <zlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "xarc_decompress.h"
#include "xarc_impl.h"
#include "filesys.h"


#if defined(_WIN32) || defined(_WIN64)
#define gzclose gzclose_r
#endif


/* Struct: d_gzip_impl
 * Extends: <xarc_decompress_impl>
 *
 * Data specific to the GZIP decompression impl.
 */
typedef struct
{
	/* Variable: base
	 * The base <xarc_decompress_impl> object.
	 */
	xarc_decompress_impl base;
	/* Variable: infile
	 * The ZLIB decompression object.
	 */
	gzFile infile;
#if XARC_NATIVE_WCHAR
	/* Variable: localized_error
	 * Holds the localized return value of <d_gzip_error_desc> when the native
	 * character size is wider than a char.
	 */
	xchar* localized_error;
#endif
} d_gzip_impl;
#define D_GZIP(base) ((d_gzip_impl*)base)


/* Section: GZIP decompression wrappers
 * See also: <decomp_open_func>, <xarc_decompress_impl>
 */


xarc_result_t d_gzip_open(xarc* x, const xchar* path,
 xarc_decompress_impl** impl);
void d_gzip_close(xarc_decompress_impl* impl);
xarc_result_t d_gzip_read(xarc* x, xarc_decompress_impl* impl, void* buf,
 size_t* read_inout);
const xchar* d_gzip_error_desc(xarc_decompress_impl* impl, int32_t error_id);


/* Link d_gzip_open as the opener function for the decomp_gzip module. */
XARC_DEFINE_DECOMPRESSOR(decomp_gzip, d_gzip_open)


/* Function: d_gzip_open
 *
 * Open a file for GZIP decompression.
 *
 * See also: <decomp_open_func>
 */
xarc_result_t d_gzip_open(xarc* x, const xchar* path,
 xarc_decompress_impl** impl)
{
	/* Open the file for reading */
	int fd = filesys_read_open(path);
	if (fd == -1)
	{
		return xarc_set_error_filesys(x, XC("Failed to open '%s' for reading"),
		 path);
	}

	/* Open a gzip input stream on the file */
	gzFile infile = gzdopen(fd, "rb");
	if (!infile)
	{
		return xarc_set_error(x, XARC_ERR_UNRECOGNIZED_COMPRESSION, 0,
		 XC("Failed to open gzip input stream on '%s'"), path);
	}

	/* Allocate and fill out a d_gzip_impl object */
	d_gzip_impl* i = (d_gzip_impl*)malloc(sizeof(d_gzip_impl));
	i->base.close = d_gzip_close;
	i->base.read = d_gzip_read;
	i->base.error_desc = d_gzip_error_desc;
	i->infile = infile;
#if XARC_NATIVE_WCHAR
	i->localized_error = 0;
#endif
	*impl = (xarc_decompress_impl*)i;

	return XARC_OK;
}


/* Function: d_gzip_close
 *
 * Close a previously opened GZIP file.
 *
 * See also: <xarc_decompress_impl>
 */
void d_gzip_close(xarc_decompress_impl* impl)
{
	/* Close the GZIP file */
	gzclose(D_GZIP(impl)->infile);
	/* Free heap memory */
#if XARC_NATIVE_WCHAR
	if (D_GZIP(impl)->localized_error)
		free(D_GZIP(impl)->localized_error);
#endif
	free(impl);
}


/* Function: d_gzip_read
 *
 * Read data from an opened GZIP file.
 *
 * See also: <xarc_decompress_impl>
 */
xarc_result_t d_gzip_read(xarc* x, xarc_decompress_impl* impl, void* buf,
 size_t* read_inout)
{
	int read = gzread(D_GZIP(impl)->infile, buf, (unsigned)(*read_inout));

	/* For any result less than 0, return due to an unrecoverable decompression
	 * error.
	 */
	if (read < 0)
	{
		int errnum;
		const char* edesc = gzerror(D_GZIP(impl)->infile, &errnum);
		if (errnum == Z_ERRNO)
		{
			return xarc_set_error_filesys(x,
			 XC("Error while reading GZIP data"));
		}
		else
		{
#if XARC_NATIVE_WCHAR
			/* Localize the ZLIB error string */
			size_t lenl = filesys_localize_char(edesc, -1, 0, 0);
			xchar* edescl = malloc(sizeof(xchar) * lenl);
			filesys_localize_char(edesc, -1, edescl, lenl);
			xarc_set_error(x, XARC_DECOMPRESS_ERROR, errnum, edescl);
			free(edescl);
			return XARC_DECOMPRESS_ERROR;
#else
			return xarc_set_error(x, XARC_DECOMPRESS_ERROR, errnum, edesc);
#endif
		}
	}

	/* Otherwise, if we read less than the amount requested, return
	 * XARC_DECOMPRESS_EOF.
	 */
	if ((size_t)read < *read_inout)
	{
		*read_inout = read;
		return xarc_set_error(x, XARC_DECOMPRESS_EOF, 0,
		 XC("EOF while reading GZIP data"));
	}

	/* If we got here, everything is fine. */
	*read_inout = (size_t)read;
	return XARC_OK;
}


/* Function: d_gzip_error_desc
 *
 * Return a human-comprehensible string describing the most recent GZIP library
 * error.
 *
 * See also: <xarc_decompress_impl>
 */
const xchar* d_gzip_error_desc(xarc_decompress_impl* impl,
 int32_t error_id __attribute__((unused)))
{
	int errnum;
	const char* edesc = gzerror(D_GZIP(impl)->infile, &errnum);
	/* Z_ERRNO in errnum indicates an error in the standard I/O library; 
	 * anything else indicates a ZLIB-specific error.
	 */
	if (errnum == Z_ERRNO)
		edesc = strerror(errno);

#if XARC_NATIVE_WCHAR
	/* Localize error string */
	size_t lenl = filesys_localize_char(edesc, -1, 0, 0);
	D_GZIP(impl)->localized_error = realloc(D_GZIP(impl)->localized_error,
	 sizeof(xchar) * lenl);
	filesys_localize_char(edesc, -1, D_GZIP(impl)->localized_error, lenl);
	return D_GZIP(impl)->localized_error;
#else
	return edesc;
#endif
}
