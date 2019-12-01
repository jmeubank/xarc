/* File: libxarc/decomp_xz/decomp_xz.c
 * Implements XZ decompression.
 */

/* Copyright 2019 John Eubank.

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
#include <7zAlloc.h>
#include <7zCrc.h>
#include <Xz.h>
#include <XzCrc64.h>
#include "xarc_impl.h"
#include "xarc_decompress.h"


#define INBUFSIZE 4096 /* The number of bytes to read in at a time */


/* Struct: d_xz_impl
 * Extends: <xarc_decompress_impl>
 *
 * Data specific to the XZ decompression impl.
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
	/* Variable: xzunpack
	 * The XZ decompression object.
	 */
	CXzUnpacker xzunpack;
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
} d_xz_impl;
#define D_XZ(base) ((d_xz_impl*)base)


/* Variable: g_Alloc
 * 7-zip memory allocation functions.
 */
static ISzAlloc g_Alloc = { SzAlloc, SzFree };


/* Section: XZ decompression wrappers
 *
 * See also: <decomp_open_func>, <xarc_decompress_impl>
 */


xarc_result_t d_xz_open(xarc* x, const xchar* path,
 xarc_decompress_impl** impl);
void d_xz_close(xarc_decompress_impl* impl);
xarc_result_t d_xz_read(xarc* x, xarc_decompress_impl* impl, void* buf,
 size_t* read_inout);
const xchar* d_xz_error_desc(xarc_decompress_impl* impl, int32_t error_id);


/* Link d_xz_open as the opener function for the decomp_xz module. */
XARC_DEFINE_DECOMPRESSOR(decomp_xz, d_xz_open)


/* Function: d_xz_open
 *
 * Open a file for XZ decompression.
 */
xarc_result_t d_xz_open(xarc* x, const xchar* path,
 xarc_decompress_impl** impl)
{
	/* Initialize 7-zip's CRC tables */
	CrcGenerateTable();
	Crc64GenerateTable();

	/* Open the file for reading */
	FILE* infile = xfopen(path, XC("rb"));
	if (!infile)
	{
		return xarc_set_error_filesys(x, XC("Failed to open '%s' for reading"),
		 path);
	}

	/* Open a decompression stream */
	CXzUnpacker xzunpack;
	XzUnpacker_Construct(&xzunpack, &g_Alloc);
	XzUnpacker_Init(&xzunpack);
	XzUnpacker_SetOutBuf(&xzunpack, NULL, 0);
	xzunpack.decodeToStreamSignature = False;

	/* Allocate and fill out a d_xz_impl object */
	d_xz_impl* i = (d_xz_impl*)malloc(sizeof(d_xz_impl));
	i->base.close = d_xz_close;
	i->base.read = d_xz_read;
	i->base.error_desc = d_xz_error_desc;
	i->infile = infile;
	i->xzunpack = xzunpack;
	i->inbuf_at = 0;
	i->inbuf_filled = 0;
	*impl = (xarc_decompress_impl*)i;

	return XARC_OK;
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
	XzUnpacker_Free(&(D_XZ(impl))->xzunpack);
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
	int srcFinished = 0;
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
		if (D_XZ(impl)->inbuf_at >= D_XZ(impl)->inbuf_filled && !srcFinished)
		{
			/* Reset the consumed count to 0 and try to fill the input buffer up
			 * to INBUFSIZE.
			 */
			D_XZ(impl)->inbuf_at = 0;
			D_XZ(impl)->inbuf_filled = fread(D_XZ(impl)->inbuf, 1, INBUFSIZE, D_XZ(impl)->infile);
			/* If we read 0 bytes, either we were already at input EOF and just
			 * didn't know it yet, or there was an error. In the case of input
			 * EOF, no more data is available and the input buffer has been
			 * completely consumed, so we're done and we return EOF.
			 */
			if (D_XZ(impl)->inbuf_filled < INBUFSIZE)
			{
				if (feof(D_XZ(impl)->infile) == 0)
				{
					return xarc_set_error_filesys(x,
				  		XC("Error while reading from file for XZ decompression"));
				}
				srcFinished = 1;
			}
		}

		/* At this point, we have (to_get - *read_inout) bytes left to try to
		 * produce, and (inbuf_filled - inbuf_at) bytes are in the input buffer
		 * and have not yet been consumed.
		 */
		size_t outbuffered = to_get - (*read_inout);
		size_t inbuffered = D_XZ(impl)->inbuf_filled - D_XZ(impl)->inbuf_at;
		ECoderStatus status;
		/* Run the decompressor. Once it returns, outbuffered will contain the
		 * number of decompressed bytes actually produced, and inbuffered will
		 * contain the number of bytes consumed from the input buffer.
		 */
		SRes ret = XzUnpacker_Code(&D_XZ(impl)->xzunpack, buf + *read_inout, &outbuffered,
    		D_XZ(impl)->inbuf + D_XZ(impl)->inbuf_at, &inbuffered, srcFinished,
    		CODER_FINISH_ANY, &status);
		*read_inout += outbuffered;
		D_XZ(impl)->inbuf_at += inbuffered;
		if (srcFinished && outbuffered == 0)
		{
			return xarc_set_error(x, XARC_DECOMPRESS_EOF, 0,
				XC("EOF while reading XZ data"));
		}
		/* If there was an error in the XZ decompressor, return immediately.
		 */
		if (ret != SZ_OK)
		{
			return xarc_set_error(x, XARC_DECOMPRESS_ERROR, ret,
			 XC("Error while reading XZ data"));
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
		case SZ_ERROR_MEM:
			return XC("Out of memory");
		case SZ_ERROR_DATA:
			return XC("File is corrupt");
		case SZ_ERROR_UNSUPPORTED:
			return XC("Unsupported XZ method or method properties");
		case SZ_ERROR_CRC:
			return XC("CRC error");
		case SZ_ERROR_NO_ARCHIVE:
			return XC("XZ invalid stream signature or invalid header");
		default:
			return XC("Internal error (bug)");
	}
	return XC("[undefined XZ error]");
}
