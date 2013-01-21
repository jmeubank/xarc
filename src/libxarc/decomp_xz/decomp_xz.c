/* File: libxarc/decomp_xz/decomp_xz.c
 * Implements LZMA and XZ decompression.
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


#include <inttypes.h>
#include <malloc.h>
#include <lzma.h>
#include "xarc_impl.h"
#include "xarc_decompress.h"


#define INBUFSIZE 4096 /* The number of bytes to read in at a time */

/* Struct: d_xz_impl
 * Extends: <xarc_decompress_impl>
 *
 * Data specific to the XZ/LZMA decompression impl.
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
	/* Variable: stream
	 * The XZ/LZMA decompression object.
	 */
	lzma_stream stream;
	/* Variable: inbuf
	 * The buffer holding input data.
	 */
	unsigned char inbuf[INBUFSIZE];
	/* Variable: inbuf_at
	 * The index in inbuf up to which the decompressor has already consumed.
	 */
	uint16_t inbuf_at;
	/* Variable: inbuf_filled
	 * The index in inbuf up to which input data is available.
	 */
	uint16_t inbuf_filled;
} d_xz_impl;
#define D_XZ(base) ((d_xz_impl*)base)


/* Section: XZ decompression wrappers
 *
 * See also: <decomp_open_func>, <xarc_decompress_impl>
 */


xarc_result_t d_xz_open_xz(xarc* x, const xchar* path,
 xarc_decompress_impl** impl);
xarc_result_t d_xz_open_lzma(xarc* x, const xchar* path,
 xarc_decompress_impl** impl);
void d_xz_close(xarc_decompress_impl* impl);
xarc_result_t d_xz_read(xarc* x, xarc_decompress_impl* impl, void* buf,
 size_t* read_inout);
const xchar* d_xz_error_desc(xarc_decompress_impl* impl, int32_t error_id);


/* Link d_xz_open_xz as the opener function for the decomp_xz module. */
XARC_DEFINE_DECOMPRESSOR(decomp_xz, d_xz_open_xz)
/* Link d_xz_open_lzma as the opener function for the decomp_xz_lzma module. */
XARC_DEFINE_DECOMPRESSOR(decomp_xz_lzma, d_xz_open_lzma)


/* Function: d_xz_open
 *
 * Open a file for XZ or LZMA decompression.
 */
xarc_result_t d_xz_open(xarc* x, const xchar* path,
 xarc_decompress_impl** impl, int xz_mode)
{
	/* Open the file for reading */
	FILE* infile = xfopen(path, XC("rb"));
	if (!infile)
	{
		return xarc_set_error_filesys(x, XC("Failed to open '%s' for reading"),
		 path);
	}

	/* Create an LZMA decompression stream */
	lzma_stream stream = LZMA_STREAM_INIT;
	lzma_ret ret = (xz_mode) ?
	 lzma_stream_decoder(&stream, UINT64_MAX, LZMA_CONCATENATED)
	 :
	 lzma_alone_decoder(&stream, UINT64_MAX);
	if (ret != LZMA_OK)
	{
		fclose(infile);
		if (ret == LZMA_MEM_ERROR)
		{
			return xarc_set_error(x, XARC_DECOMPRESS_ERROR, ret,
			 XC("Out of memory"));
		}
		else
		{
			return xarc_set_error(x, XARC_DECOMPRESS_ERROR, ret,
			 XC("XZ/LZMA decompressor internal error"));
		}
	}

	/* Allocate and fill out a d_xz_impl object */
	d_xz_impl* i = (d_xz_impl*)malloc(sizeof(d_xz_impl));
	i->base.close = d_xz_close;
	i->base.read = d_xz_read;
	i->base.error_desc = d_xz_error_desc;
	i->infile = infile;
	i->stream = stream;
	i->inbuf_at = 0;
	i->inbuf_filled = 0;
	*impl = (xarc_decompress_impl*)i;

	return XARC_OK;
}

/* Function: d_xz_open_lzma
 *
 * Open a file for LZMA decompression.
 *
 * See also: <decomp_open_func>
 */
xarc_result_t d_xz_open_lzma(xarc* x, const xchar* path,
 xarc_decompress_impl** impl)
{
	return d_xz_open(x, path, impl, 0);
}

/* Function: d_xz_open_xz
 *
 * Open a file for XZ decompression.
 *
 * See also: <decomp_open_func>
 */
xarc_result_t d_xz_open_xz(xarc* x, const xchar* path,
 xarc_decompress_impl** impl)
{
	return d_xz_open(x, path, impl, 1);
}

/* Function: d_xz_close
 *
 * Close a previously opened XZ/LZMA file.
 *
 * See also: <xarc_decompress_impl>
 */
void d_xz_close(xarc_decompress_impl* impl)
{
	/* Close the XZ decompressor */
	lzma_end(&(D_XZ(impl))->stream);
	/* Close the input file */
	fclose(D_XZ(impl)->infile);
	/* Free heap memory */
	free(impl);
}

/* Function: d_xz_read
 *
 * Read data from an opened XZ file.
 *
 * See also: <xarc_decompress_impl>
 */
xarc_result_t d_xz_read(xarc* x, xarc_decompress_impl* impl, void* buf,
 size_t* read_inout)
{
	size_t to_get = *read_inout;
	*read_inout = 0;
	lzma_action action = LZMA_RUN;
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
		if (/* action == LZMA_RUN && */
		 D_XZ(impl)->inbuf_at >= D_XZ(impl)->inbuf_filled)
		{
			/* Reset the consumed count to 0 and try to fill the input
			 * buffer up to INBUFSIZE.
			 */
			D_XZ(impl)->inbuf_at = 0;
			D_XZ(impl)->inbuf_filled =
			 fread(D_XZ(impl)->inbuf, 1, INBUFSIZE, D_XZ(impl)->infile);
			/* If we've reached EOF, no more data is available for input and
			 * we will try to finish the stream. Otherwise, if we got 0 bytes
			 * back, there was an error.
			 */
			if (feof(D_XZ(impl)->infile))
			{
				/* action = LZMA_FINISH; */
			}
			else if (D_XZ(impl)->inbuf_filled == 0)
			{
				return xarc_set_error_filesys(x,
				 XC("Error while reading from file for XZ decompression"));
			}
		}

		/* At this point, we have (to_get - *read_inout) bytes left to try to
		 * produce, and (inbuf_filled - inbuf_at) bytes are in the input buffer
		 * and have not yet been consumed.
		 */
		D_XZ(impl)->stream.next_in = D_XZ(impl)->inbuf + D_XZ(impl)->inbuf_at;
		D_XZ(impl)->stream.avail_in = D_XZ(impl)->inbuf_filled - D_XZ(impl)->inbuf_at;
		D_XZ(impl)->stream.total_in = 0uL;
		D_XZ(impl)->stream.next_out = buf + *read_inout;
		D_XZ(impl)->stream.avail_out = to_get - (*read_inout);
		D_XZ(impl)->stream.total_out = 0uL;
		/* Run the decompressor. Once it returns, stream.total_out will contain
		 * the number of decompressed bytes actually produced, and
		 * stream.total_in will contain the number of bytes consumed from the
		 * input buffer.
		 */
		lzma_ret ret = lzma_code(&(D_XZ(impl)->stream), action);
		D_XZ(impl)->inbuf_at += D_XZ(impl)->stream.total_in;
		*read_inout += D_XZ(impl)->stream.total_out;
		if (ret != LZMA_OK)
		{
			if (/* action == LZMA_FINISH && */ ret == LZMA_STREAM_END)
			{
				return xarc_set_error(x, XARC_DECOMPRESS_EOF, 0,
				 XC("EOF while reading XZ data"));
			}
			else
			{
				return xarc_set_error(x, XARC_DECOMPRESS_ERROR, ret,
				 XC("Error while reading XZ data"));
			}
		}
	}

	/* Here, the full number of requested decompressed bytes have been produced.
	 */
	return XARC_OK;
}

/* Function: d_xz_error_desc
 *
 * Return a human-comprehensible string describing the most
 * recent XZ library error.
 *
 * See also: <xarc_decompress_impl>
 */
const xchar* d_xz_error_desc(xarc_decompress_impl* impl __attribute__((unused)),
 int32_t error_id)
{
	switch (error_id)
	{
	case LZMA_MEM_ERROR:
		return XC("Out of memory");
	case LZMA_FORMAT_ERROR:
		return XC("File format not recognized");
	case LZMA_OPTIONS_ERROR:
		return XC("Unsupported compression options");
	case LZMA_DATA_ERROR:
		return XC("File is corrupt");
	case LZMA_BUF_ERROR:
		return XC("Unexpected end of input");
	default:
		return XC("Internal error (bug)");
	}
	return XC("[undefined LZMA error]");
}
