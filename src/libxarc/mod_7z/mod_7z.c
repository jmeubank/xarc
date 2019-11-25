/* File: libxarc/mod_7z/mod_7z.c
 * 7-zip archive module.
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


#include <7z.h>
#include <7zCrc.h>
#include <7zFile.h>
#include <inttypes.h>
#include <malloc.h>
#include <string.h>
#include <7zAlloc.h>
#include "filesys.h"
#include "xarc_impl.h"


/* Struct: m_7z_extra
 * typedef struct {...} m_7z_extra - Data specific to the 7-zip archive module.
 */
typedef struct
{
	/* Field: instream
	 * 7-zip file reading impl.
	 */
	CFileInStream instream;
	/* Field: lookstream
	 * 7-zip seeking impl.
	 */
	CLookToRead2 lookstream;
	/* Field: db
	 * 7-zip archive database.
	 */
	CSzArEx db;
	/* Field: entry
	 * Index in archive database of current item.
	 */
	uint32_t entry;
	/* Field: entry_path
	 * Relative path of current item.
	 *
	 * NULL unless <m_7z_item_get_info> has been called for the current item.
	 */
	xchar* entry_path;
	/* Field: block_index
	 * 7-zip cached block index
	 *
	 * Maintained by 7-zip decompressor as a cache until the archive is closed.
	 */
	UInt32 block_index;
	/* Field: out_buffer
	 * Stores output from decompressing a file entry in the archive.
	 *
	 * Maintained by 7-zip decompressor as a cache until the archive is closed.
	 */
	Byte *out_buffer;
	/* Field: out_buffer_size
	 * Size of the buffer pointed to by <out_buffer>.
	 *
	 * Maintained by 7-zip decompressor as a cache until the archive is closed.
	 */
	size_t out_buffer_size;
} m_7z_extra;
#define M_7Z(x) ((m_7z_extra*)((void*)x + sizeof(struct _xarc)))


/* Section: Global Data */


xarc_result_t m_7z_open(xarc* x, const xchar* file, uint8_t type);
xarc_result_t m_7z_close(xarc* x);
xarc_result_t m_7z_next_item(xarc* x);
xarc_result_t m_7z_item_get_info(xarc* x, xarc_item_info* info);
xarc_result_t m_7z_item_extract(xarc* x, FILE* to, size_t* written);
xarc_result_t m_7z_item_set_props(xarc* x, const xchar* path);
const xchar* m_7z_error_description(xarc* x, int32_t error_id);


/* Link m_7z_open as the opener function for the mod_7z archive module. */
XARC_DEFINE_MODULE(mod_7z, m_7z_open, sizeof(m_7z_extra))


/* Variable: sz_funcs
 * Implements the <handler_funcs> interface for mod_7z
 */
handler_funcs sz_funcs = {
	m_7z_close,
	m_7z_next_item,
	m_7z_item_get_info,
	m_7z_item_extract,
	m_7z_item_set_props,
	m_7z_error_description
};


/* 7-zip lets us specify custom functions for allocating and freeing shared
 * memory (passed out from the 7-zip library) and temporary memory (used and
 * freed within the 7-zip library). We'll just go with 7-zip's own functions.
 */
ISzAlloc g_alloc = { SzAlloc, SzFree };
ISzAlloc g_alloc_temp = { SzAllocTemp, SzFreeTemp };

/* Variable: sz_error_names
 * Maps 7-zip's integer error IDs to string versions of their defined constant
 * identifiers.
 */
static const xchar* sz_error_names[] = {
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
 XC("SZ_ERROR_THREAD"),
 XC("[undefined]"),
 XC("[undefined]"),
 XC("[undefined]"),
 XC("SZ_ERROR_ARCHIVE"),
 XC("SZ_ERROR_NO_ARCHIVE")
};


/* Section: Global Functions */


/* 7-zip has its own support for wide/narrow-char file paths */
static WRes InFile_OpenX(CSzFile *p, const xchar *name)
{
#if XARC_NATIVE_WCHAR
	return InFile_OpenW(p, name);
#else
	return InFile_Open(p, name);
#endif
}


/* Section: Static variables */
static const size_t kInputBufSize = (size_t) 1 << 18;


/* Function: m_7z_open
 * Open a file as a 7-zip archive.
 *
 * See also: <XARC_DEFINE_MODULE(name, open_func, extra_size)>
 */
xarc_result_t m_7z_open(xarc* x, const xchar* file, uint8_t type __attribute__((unused)))
{
	/* Clear our own allocated space */
	memset(M_7Z(x), 0, sizeof(m_7z_extra));
	/* Set this <xarc> object to use the 7-zip implementation of <handler_funcs>
	 */
	X_BASE(x)->impl = &sz_funcs;

	/* Try to open the file for reading */
	if (InFile_OpenX(&M_7Z(x)->instream.file, file))
	{
		return xarc_set_error(x, XARC_ERR_NOT_VALID_ARCHIVE, 0,
		 XC("Failed to open '%s' for reading"), file);
	}
	/* Set up the 7-zip stream object as a FileInStream */
	FileInStream_CreateVTable(&M_7Z(x)->instream);

	/* Set up the 7-zip seek object as a standard LookToRead */
	LookToRead2_CreateVTable(&M_7Z(x)->lookstream, False);
	M_7Z(x)->lookstream.buf = NULL;

	/* Allocate memory buffer for decompression */
    M_7Z(x)->lookstream.buf = (Byte*)ISzAlloc_Alloc(&g_alloc, kInputBufSize);
    if (!M_7Z(x)->lookstream.buf)
	{
    	return xarc_set_error(x, XARC_ERR_MEMORY, 0,
		 XC("Failed allocating %"PRIu32" bytes of memory for decompression - out of memory?"),
		 kInputBufSize);
	}

	/* Use the opened file stream with this seek object */
	M_7Z(x)->lookstream.bufSize = kInputBufSize;
	M_7Z(x)->lookstream.realStream = &M_7Z(x)->instream.vt;
	LookToRead2_Init(&M_7Z(x)->lookstream);

	/* Initialize 7-zip's CRC table */
	CrcGenerateTable();

	/* Initialize the 7-zip database object */
	SzArEx_Init(&M_7Z(x)->db);
	/* Try to open the file stream as a 7-zip archive */
	if (SzArEx_Open(&M_7Z(x)->db, &M_7Z(x)->lookstream.vt, &g_alloc,
	 &g_alloc_temp) != SZ_OK)
	{
		File_Close(&M_7Z(x)->instream.file);
		return xarc_set_error(x, XARC_ERR_NOT_VALID_ARCHIVE, 0,
		 XC("Failed to open '%s' as a 7z archive"), file);
	}

	return XARC_OK;
}

/* Function: m_7z_close
 * Close an open 7-zip archive.
 *
 * See also: <handler_funcs.close>
 */
xarc_result_t m_7z_close(xarc* x)
{
	if (M_7Z(x)->out_buffer)
		IAlloc_Free(&g_alloc, M_7Z(x)->out_buffer);
	if (M_7Z(x)->entry_path)
		free(M_7Z(x)->entry_path);
	SzArEx_Free(&M_7Z(x)->db, &g_alloc);
	File_Close(&M_7Z(x)->instream.file);
	return XARC_OK;
}

/* Function: m_7z_next_item
 * Seek to the next item in a 7-zip archive.
 *
 * See also: <handler_funcs.next_item>
 */
xarc_result_t m_7z_next_item(xarc* x)
{
	/* If we're already at the last entry, return XARC_NO_MORE_ITEMS */
	if (M_7Z(x)->entry + 1 >= M_7Z(x)->db.NumFiles)
	{
		return xarc_set_error(x, XARC_NO_MORE_ITEMS, 0,
		 XC("End of 7z archive"));
	}
	/* Increment the entry index */
	++(M_7Z(x)->entry);
	/* If we had retrieved the last entry's path, free it now */
	if (M_7Z(x)->entry_path)
	{
		free(M_7Z(x)->entry_path);
		M_7Z(x)->entry_path = 0;
	}
	return XARC_OK;
}

/* Function: m_7z_item_get_info
 * Get the metadata for the current item.
 *
 * See also: <handler_funcs.item_get_info>
 */
xarc_result_t m_7z_item_get_info(xarc* x, xarc_item_info* info)
{
	/* If we haven't previously retrieved the current item's path, retrieve it
	 * now.
	 */
	if (!M_7Z(x)->entry_path)
	{
		/* Get the length of the path (in characters) */
		size_t len16 = SzArEx_GetFileNameUtf16(&M_7Z(x)->db, M_7Z(x)->entry, 0);
#if XARC_NATIVE_WCHAR
		/* For now, assume XARC_NATIVE_WCHAR means UTF-16 */
		M_7Z(x)->entry_path = malloc(sizeof(uint16_t) * len16);
		SzArEx_GetFileNameUtf16(&M_7Z(x)->db, M_7Z(x)->entry,
		 M_7Z(x)->entry_path);
#else
		/* If native character type isn't wchar_t, convert to char */
		uint16_t* path16 = malloc(sizeof(uint16_t) * len16);
		SzArEx_GetFileNameUtf16(&M_7Z(x)->db, M_7Z(x)->entry, path16);
		size_t lenx = filesys_localize_utf16(path16, len16, 0, 0);
		M_7Z(x)->entry_path = malloc(sizeof(xchar) * lenx);
		filesys_localize_utf16(path16, len16, M_7Z(x)->entry_path, lenx);
		free(path16);
#endif
	}
	info->path = M_7Z(x)->entry_path;

	/* Check if the entry is a directory */
	if (SzArEx_IsDir(&(M_7Z(x)->db), M_7Z(x)->entry))
		info->properties = XARC_PROP_DIR;
	else
		info->properties = 0;

	/* Check if the entry has a modification time defined */
	if (SzBitWithVals_Check(&(M_7Z(x)->db.MTime), M_7Z(x)->entry))
	{
		filesys_time_winft((const win_filetime*)&M_7Z(x)->db.MTime.Vals[M_7Z(x)->entry],
		 &info->mod_time);
	}

	return XARC_OK;
}

/* Function: m_7z_item_extract
 * Extract the current file item to an open FILE stream.
 *
 * See also: <handler_funcs.item_extract>
 */
xarc_result_t m_7z_item_extract(xarc* x, FILE* to, size_t* written)
{
	/* 7-zip will store the offset within the output buffer cache where it has
	 * placed the decompressed data and where we may start reading from.
	 */
	size_t offset;
	/* 7-zip will store the amount of data (in bytes) it has placed in the
	 * buffer.
	 */
	size_t out_processed;
	/* Unpack the entire file at once into memory. 7-zip will take care of
	 * allocating the necessary memory. TODO: This is obviously broken for large
	 * enough files.
	 */
	SRes res = SzArEx_Extract(&M_7Z(x)->db, &M_7Z(x)->lookstream.vt,
	 M_7Z(x)->entry, &M_7Z(x)->block_index, &M_7Z(x)->out_buffer,
	 &M_7Z(x)->out_buffer_size, &offset, &out_processed, &g_alloc,
	 &g_alloc_temp);
	if (res != SZ_OK)
	{
		return xarc_set_error(x, XARC_MODULE_ERROR, res,
		 XC("7zlib failed to unpack file"));
	}
	/* Write out the unpacked data to the FILE stream. */
	int wr = fwrite(M_7Z(x)->out_buffer + offset, 1, out_processed, to);
	if (wr <= 0 || (size_t)wr != out_processed)
	{
		return xarc_set_error_filesys(x, XC("Failed to write %"PRIuMAX" bytes"),
		 out_processed);
	}
	/* Store the amount of data written */
	*written = out_processed;
	return XARC_OK;
}

/* Function: m_7z_item_set_props
 * Apply the entry's metadata to the written file.
 *
 * See also: <handler_funcs.item_set_props>
 */
xarc_result_t m_7z_item_set_props(xarc* x, const xchar* path)
{
	/* If the item has Windows-style attributes stored, apply them (Unix systems
	 * will reinterpret as much as possible).
	 */
	if (SzBitWithVals_Check(&(M_7Z(x)->db.Attribs), M_7Z(x)->entry))
	{
		filesys_set_attributes_win(path,
		 M_7Z(x)->db.Attribs.Vals[M_7Z(x)->entry]);
	}
	/* If the item has a last-modified time, apply it. */
	if (SzBitWithVals_Check(&(M_7Z(x)->db.MTime), M_7Z(x)->entry))
	{
		filesys_set_modtime_winft(path,
		 (const win_filetime*)&M_7Z(x)->db.MTime.Vals[M_7Z(x)->entry]);
	}
	return XARC_OK;
}

/* Function: m_7z_error_description
 * Return a string describing the supplied integer error id.
 *
 * See also: <handler_funcs.error_description>
 */
const xchar* m_7z_error_description(xarc* x __attribute__((unused)),
 int32_t error_id)
{
	/* If the ID is outside the range used by 7-zip, something odd has happened.
	 */
	if (error_id < 1 || error_id > 17)
		return XC("[undefined]");
	/* Otherwise, return the string indexed in the <sz_error_names> table. */
	return sz_error_names[error_id - 1];
}
