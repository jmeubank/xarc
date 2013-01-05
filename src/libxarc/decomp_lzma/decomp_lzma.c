/* File: libxarc/decomp_lzma/decomp_lzma.c
 * Implements LZMA decompression.
 */
/* Created: JohnE, 2010-08-08 */


#include <stdio.h>
#include <inttypes.h>
#include <malloc.h>
#include <LzmaDec.h>
#include <Util/7z/7zAlloc.h>
#include "xarc_impl.h"
#include "xarc_decompress.h"


#define INBUFSIZE 4096 /* The number of bytes to read in at a time */

/* Struct: d_lzma_impl
 * Extends: <xarc_decompress_impl>
 *
 * Data specific to the LZMA decompression impl.
 */
typedef struct
{
	/* Variable: base
	 * The base xarc_decompress_impl object.
	 */
	xarc_decompress_impl base;
	/* Variable: infile
	 * The stdio stream for reading the archive file.
	 */
	FILE* infile;
	/* Variable: lzdecomp
	 * The LZMA decompression object.
	 */
	CLzmaDec lzdecomp;
	/* Variable: inbuf
	 * The buffer holding input data.
	 */
	Byte inbuf[INBUFSIZE];
	/* Variable: inbuf_at
	 * The index in inbuf up to which the decompressor has already consumed.
	 */
	uint16_t inbuf_at;
	/* Variable: inbuf_filled
	 * The index in inbuf up to which input data is available.
	 */
	uint16_t inbuf_filled;
} d_lzma_impl;
#define D_LZMA(base) ((d_lzma_impl*)base)


/* Section: LZMA decompression wrappers
 *
 * See also: <decomp_open_func>, <xarc_decompress_impl>
 */


xarc_result_t d_lzma_open(xarc* x, const xchar* path,
 xarc_decompress_impl** impl);
void d_lzma_close(xarc_decompress_impl* impl);
xarc_result_t d_lzma_read(xarc* x, xarc_decompress_impl* impl, void* buf,
 size_t* read_inout);
const xchar* d_lzma_error_desc(xarc_decompress_impl* impl, int32_t error_id);


/* Link d_lzma_open as the opener function for the decomp_lzma module. */
XARC_DEFINE_DECOMPRESSOR(decomp_lzma, d_lzma_open)


/* Variable: g_Alloc
 * 7-zip memory allocation functions.
 */
ISzAlloc g_Alloc = { SzAlloc, SzFree };
/* Variable: lzma_error_strings
 * 7-zip error strings.
 */
const xchar* lzma_error_strings[] = {
	XC("SZ_ERROR_DATA"),
	XC("SZ_ERROR_MEM"),
	XC("SZ_ERROR_CRC"),
	XC("SZ_ERROR_UNSUPPORTED"),
	XC("SZ_ERROR_PARAM"),
	XC("SZ_ERROR_INPUT_EOF"),
	XC("SZ_ERROR_OUTPUT_EOF"),
	XC("SZ_ERROR_READ"),
	XC("SZ_ERROR_WRITE"),
	XC("SZ_ERROR_PROGRESS"),
	XC("SZ_ERROR_FAIL"),
	XC("SZ_ERROR_THREAD")
};


/* Function: d_lzma_open
 *
 * Open a file for LZMA decompression.
 *
 * See also: <decomp_open_func>
 */
xarc_result_t d_lzma_open(xarc* x, const xchar* path,
 xarc_decompress_impl** impl)
{
	/* Open the file for reading */
	FILE* infile = xfopen(path, XC("rb"));
	if (!infile)
	{
		return xarc_set_error_filesys(x, XC("Failed to open '%s' for reading"),
		 path);
	}

	/* Read the LZMA header -- stream properties plus 8 bytes uncompressed size
	 */
	uint8_t lzheader[LZMA_PROPS_SIZE + 8];
	if (fread(lzheader, 1, LZMA_PROPS_SIZE + 8, infile) != LZMA_PROPS_SIZE + 8)
	{
		fclose(infile);
		return xarc_set_error(x, XARC_DECOMPRESS_ERROR, SZ_ERROR_UNSUPPORTED,
		 XC("EOF reading LZMA stream properties from '%s'"), path);
	}

	/* Open a LZMA decompression stream */
	CLzmaDec lzdecomp;
	LzmaDec_Construct(&lzdecomp);
	SRes ret = LzmaDec_Allocate(&lzdecomp, lzheader, LZMA_PROPS_SIZE, &g_Alloc);
	if (ret != SZ_OK)
	{
		fclose(infile);
		return xarc_set_error(x, XARC_DECOMPRESS_ERROR, ret,
		 XC("Error initializing LZMA decompressor for '%s'"), path);
	}
	LzmaDec_Init(&lzdecomp);

	/* Allocate and fill out a d_lzma_impl object */
	d_lzma_impl* i = (d_lzma_impl*)malloc(sizeof(d_lzma_impl));
	i->base.close = d_lzma_close;
	i->base.read = d_lzma_read;
	i->base.error_desc = d_lzma_error_desc;
	i->infile = infile;
	i->lzdecomp = lzdecomp;
	i->inbuf_at = 0;
	i->inbuf_filled = 0;
	*impl = (xarc_decompress_impl*)i;

	return XARC_OK;
}

/* Function: d_lzma_close
 *
 * Close a previously opened LZMA file.
 *
 * See also: <xarc_decompress_impl>
 */
void d_lzma_close(xarc_decompress_impl* impl)
{
	/* Close the LZMA decompressor */
	LzmaDec_Free(&D_LZMA(impl)->lzdecomp, &g_Alloc);
	/* Close the input file */
	fclose(D_LZMA(impl)->infile);
	/* Free heap memory */
	free(impl);
}

/* Function: d_lzma_read
 *
 * Read data from an opened LZMA file.
 *
 * See also: <xarc_decompress_impl>
 */
xarc_result_t d_lzma_read(xarc* x, xarc_decompress_impl* impl, void* buf,
 size_t* read_inout)
{
	size_t to_get = *read_inout;
	*read_inout = 0;
	/* The LZMA decompressor requires us to manage the input buffer ourselves in
	 * addition to the output buffer. Since the decompressor may not be able to
	 * produce all the output we need in one pass at the input buffer, we need
	 * to loop until either we finally do have all the output we need, or we've
	 * reached EOF.
	 */
	while (*read_inout < to_get)
	{
		/* If the decompressor has consumed all the data in the input buffer,
		 * try to refill it.
		 */
		if (D_LZMA(impl)->inbuf_at >= D_LZMA(impl)->inbuf_filled)
		{
			/* If we've previously reached input EOF, no more data is available.
			 * Since the decompressor has already consumed the entire buffer, we
			 * are done and we return EOF.
			 */
			if (feof(D_LZMA(impl)->infile))
			{
				return xarc_set_error(x, XARC_DECOMPRESS_EOF, 0,
				 XC("EOF while reading LZMA data"));
			}
			/* Reset the consumed count to 0 and try to fill the input buffer up
			 * to INBUFSIZE.
			 */
			D_LZMA(impl)->inbuf_at = 0;
			D_LZMA(impl)->inbuf_filled =
			 fread(D_LZMA(impl)->inbuf, 1, INBUFSIZE, D_LZMA(impl)->infile);
			/* If we read 0 bytes, either we were already at input EOF and just
			 * didn't know it yet, or there was an error. In the case of input
			 * EOF, no more data is available and the input buffer has been
			 * completely consumed, so we're done and we return EOF.
			 */
			if (D_LZMA(impl)->inbuf_filled == 0)
			{
				return (feof(D_LZMA(impl)->infile)) ?
				 xarc_set_error(x, XARC_DECOMPRESS_EOF, 0,
				  XC("EOF while reading LZMA data"))
				 :
				 xarc_set_error_filesys(x,
				  XC("Error while reading from file for LZMA decompression"));
			}
		}

		/* At this point, we have (to_get - *read_inout) bytes left to try to
		 * produce, and (inbuf_filled - inbuf_at) bytes are in the input buffer
		 * and have not yet been consumed.
		 */
		size_t outbuffered = to_get - (*read_inout);
		size_t inbuffered = D_LZMA(impl)->inbuf_filled - D_LZMA(impl)->inbuf_at;
		ELzmaStatus status;
		/* Run the decompressor. Once it returns, outbuffered will contain the
		 * number of decompressed bytes actually produced, and inbuffered will
		 * contain the number of bytes consumed from the input buffer.
		 */
		SRes ret = LzmaDec_DecodeToBuf(&D_LZMA(impl)->lzdecomp,
		 buf + *read_inout, &outbuffered,
		 D_LZMA(impl)->inbuf + D_LZMA(impl)->inbuf_at, &inbuffered,
		 LZMA_FINISH_ANY, &status);
		*read_inout += outbuffered;
		D_LZMA(impl)->inbuf_at += inbuffered;
		/* If there was an error in the LZMA decompressor, return immediately.
		 */
		if (ret != SZ_OK)
		{
			return xarc_set_error(x, XARC_DECOMPRESS_ERROR, ret,
			 XC("Error while reading LZMA data"));
		}
	}

	/* Here, the full number of requested decompressed bytes have been produced.
	 */
	return XARC_OK;
}

/* Function: d_lzma_error_desc
 *
 * Return a human-comprehensible string describing the most
 * recent LZMA library error.
 *
 * See also: <xarc_decompress_impl>
 */
const xchar* d_lzma_error_desc(xarc_decompress_impl* impl __attribute__((unused)),
 int32_t error_id)
{
	if (error_id < 1 || error_id > 12)
		return XC("[undefined LZMA error]");
	return lzma_error_strings[error_id - 1];
}
