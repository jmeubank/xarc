/* File: libxarc/mod_minizip/mod_minizip.c
 * Minizip archive module (handles ZIP archives).
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


#include <string.h>
#include "filesys.h"
#include "unzip.h"
#include "xarc_impl.h"

#if defined(_WIN32) || defined(_WIN64)
#include "iowin32.h"
#endif


/* Struct: m_zip_extra
 * typedef struct {...} m_zip_extra - Data specific to the minizip archive
 * module.
 */
typedef struct
{
	/* Field: file
	 * Minizip archive file object.
	 */
	unzFile file;
	/* Field: item_path
	 * Relative path of current item.
	 *
	 * NULL unless <m_zip_item_get_info> has been called for the current item.
	 */
	xchar* item_path;
#if XARC_NATIVE_WCHAR
	/* Since minizip only knows about narrow-char strings, we have to convert
	 * its error strings to wide-char and store them.
	 */
	wchar_t* localized_error;
#endif
} m_zip_extra;
#define M_ZIP(x) ((m_zip_extra*)((void*)x + sizeof(struct _xarc)))


/* Section: Global Data */


xarc_result_t m_zip_open(xarc* x, const xchar* file, uint8_t type);
xarc_result_t m_zip_close(xarc* x);
xarc_result_t m_zip_next_item(xarc* x);
xarc_result_t m_zip_item_get_info(xarc* x, xarc_item_info* info);
xarc_result_t m_zip_item_extract(xarc* x, FILE* to, size_t* written);
xarc_result_t m_zip_item_set_props(xarc* x, const xchar* path);
const xchar* m_zip_error_description(xarc* x, int32_t error_id);


/* Link m_zip_open as the opener function for the mod_minizip archive module. */
XARC_DEFINE_MODULE(mod_minizip, m_zip_open, sizeof(m_zip_extra))


/* Variable: zip_funcs
 * Implements the <handler_funcs> interface for mod_minizip
 */
handler_funcs zip_funcs = {
	m_zip_close,
	m_zip_next_item,
	m_zip_item_get_info,
	m_zip_item_extract,
	m_zip_item_set_props,
	m_zip_error_description
};

/* Variable: minizip_descriptors
 * Maps minizip's integer error IDs to strings describing the errors.
 */
const xchar* minizip_descriptors[] = {
	XC("No more items in zip file"), /* UNZ_END_OF_LIST_OF_FILE (-100) */
	XC("[invalid minizip error]"),
	XC("Parameter error calling minizip function"), /* UNZ_PARAMERROR (-102) */
	XC("Corrupt or invalid zip archive"), /* UNZ_BADZIPFILE (-103) */
	XC("Internal error in minizip"), /* UNZ_INTERNALERROR (-104) */
	XC("CRC error in zip archive") /* UNZ_CRCERROR (-105) */
};

/* Helper function to store the appropriate xarc error data based on the error
 * ID returned by a minizip function.
 */
static xarc_result_t set_error_zip(xarc* x, int mzerror)
{
	return (mzerror == UNZ_ERRNO) ?
	 xarc_set_error_filesys(x, 0) :
	 xarc_set_error(x, XARC_MODULE_ERROR, mzerror, 0);
}


/* Function: m_zip_open
 * Open a file as a ZIP archive.
 *
 * See also: <XARC_DEFINE_MODULE(name, open_func, extra_size)>
 */
xarc_result_t m_zip_open(xarc* x, const xchar* file,
 uint8_t type __attribute__((unused)))
{
	/* Clear our own allocated space */
	memset(M_ZIP(x), 0, sizeof(m_zip_extra));
	/* Set this <xarc> object to use the minizip implementation of
	 * <handler_funcs>
	 */
	X_BASE(x)->impl = &zip_funcs;

	/* Fill out a zlib_filefunc64_def object to tell minizip whether to use
	 * Windows functions or standard 64-bit fopen functions
	 */
	zlib_filefunc64_def zfuncs;
#if defined(_WIN32) || defined(_WIN64)
	fill_win32_filefunc64(&zfuncs);
#else
	fill_fopen64_filefunc(&zfuncs);
#endif
	/* Try to open the file as a ZIP archive */
	unzFile f = unzOpen2_64((const char*)file, &zfuncs);
	if (!f)
	{
		return xarc_set_error(x, XARC_ERR_NOT_VALID_ARCHIVE, 0,
		 XC("minizip failed to open '%s' as a ZIP archive"), file);
	}
	M_ZIP(x)->file = f;

	return XARC_OK;
}

/* Function: m_zip_close
 * Close an open minizip archive.
 *
 * See also: <handler_funcs.close>
 */
xarc_result_t m_zip_close(xarc* x)
{
#if XARC_NATIVE_WCHAR
	if (M_ZIP(x)->localized_error)
		free(M_ZIP(x)->localized_error);
#endif
	if (M_ZIP(x)->item_path)
		free(M_ZIP(x)->item_path);
	int ret = UNZ_OK;
	if (M_ZIP(x)->file)
		ret = unzClose(M_ZIP(x)->file);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);
	return XARC_OK;
}

/* Function: m_zip_next_item
 * Seek to the next item in a minizip archive.
 *
 * See also: <handler_funcs.next_item>
 */
xarc_result_t m_zip_next_item(xarc* x)
{
	int ret = unzGoToNextFile(M_ZIP(x)->file);
	if (ret == UNZ_END_OF_LIST_OF_FILE)
		return xarc_set_error(x, XARC_NO_MORE_ITEMS, ret, 0);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);
	return XARC_OK;
}

/* Function: m_zip_item_get_info
 * Get the metadata for the current item.
 *
 * See also: <handler_funcs.item_get_info>
 */
xarc_result_t m_zip_item_get_info(xarc* x, xarc_item_info* info)
{
	/* Get the constant-size data and the lengths of the variable-size data */
	unz_file_info ufi;
	int ret = unzGetCurrentFileInfo(M_ZIP(x)->file, &ufi, 0, 0, 0, 0, 0, 0);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);

	/* Get the stored relative path */
	char* path8 = malloc(ufi.size_filename + 1);
	ret = unzGetCurrentFileInfo(M_ZIP(x)->file, &ufi, path8,
	 ufi.size_filename + 1, 0, 0, 0, 0);
	if (ret != UNZ_OK)
	{
		free(M_ZIP(x)->item_path);
		M_ZIP(x)->item_path = 0;
		return set_error_zip(x, ret);
	}
	/* Convert the path from UTF-8 or CP-437 to the native format */
	size_t lenl;
	if (ufi.flag & 0x400) // general purpose bit 11: UTF-8 if set
	{
		lenl = filesys_localize_utf8(path8, ufi.size_filename + 1, 0, 0);
		M_ZIP(x)->item_path = realloc(M_ZIP(x)->item_path,
		 sizeof(xchar) * lenl);
		filesys_localize_utf8(path8, ufi.size_filename + 1, M_ZIP(x)->item_path,
		 lenl);
	}
	else
	{
		lenl = filesys_localize_cp437(path8, ufi.size_filename + 1, 0, 0);
		M_ZIP(x)->item_path = realloc(M_ZIP(x)->item_path,
		 sizeof(xchar) * lenl);
		filesys_localize_cp437(path8, ufi.size_filename + 1,
		 M_ZIP(x)->item_path, lenl);
	}
	free(path8);

	/* Initialize the <xarc_item_info> object */
	memset(info, 0, sizeof(xarc_item_info));

	/* Set the item's relative path */
	info->path = M_ZIP(x)->item_path;

	/* If the last character in the entry's path is a '/' ... */
	if (M_ZIP(x)->item_path[ufi.size_filename - 1] == '/'
	/* ... or the entry has the directory flag set ... */
	 || ((ufi.version >> 8 == 0 || ufi.version >> 8 == 10)
	  && (ufi.external_fa & 0x0010)))
	/* ... then set XARC's directory flag. */
		info->properties |= XARC_PROP_DIR;

	/* Store the item's last-modified timestamp */
	filesys_time_dos(ufi.dosDate >> 16, ufi.dosDate, &info->mod_time);

	return XARC_OK;
}

/* Function: m_zip_item_extract
 * Extract the current file item to an open FILE stream.
 *
 * See also: <handler_funcs.item_extract>
 */
xarc_result_t m_zip_item_extract(xarc* x, FILE* to, size_t* written)
{
	/* Tell minizip to get ready to extract the current item */
	int ret = unzOpenCurrentFile(M_ZIP(x)->file);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);

	/* Get unpacked data from minizip and write it out, 4096 bytes at a time. */
	*written = 0;
	char buf[4096];
	while (1)
	{
		ret = unzReadCurrentFile(M_ZIP(x)->file, buf, 4096);
		/* Less than 0 means an error occurred */
		if (ret < 0)
			return set_error_zip(x, ret);
		/* 0 means we reached the end of the entry */
		if (ret == 0)
			break;
		/* Write out whatever data we got */
		size_t wr = fwrite(buf, 1, ret, to);
		/* Keep <written> up-to-date on the amount of data written out */
		if (wr > 0)
			*written += wr;
		/* If we didn't write all the data we wanted to, something odd happened.
		 */
		if (wr != (size_t)ret)
			return xarc_set_error_filesys(x, 0);
	}

	/* Tell minizip we're done extracting. At this point it may verify the CRC
	 * of the extracted data, returning UNZ_CRCERROR if the CRC didn't match.
	 */
	ret = unzCloseCurrentFile(M_ZIP(x)->file);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);

	return XARC_OK;
}

/* Function: m_zip_item_set_props
 * Apply the entry's metadata to the written file.
 *
 * See also: <handler_funcs.item_set_props>
 */
xarc_result_t m_zip_item_set_props(xarc* x, const xchar* path)
{
	/* Get the entry's metadata */
	unz_file_info ufi;
	int ret = unzGetCurrentFileInfo(M_ZIP(x)->file, &ufi, 0, 0, 0, 0, 0, 0);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);

	/* If the entry contains Windows-format file attributes, apply them (Unix
	 * systems will interpret as best they can)
	 */
	if (ufi.version >> 8 == 0 || ufi.version >> 8 == 10)
		filesys_set_attributes_win(path, ufi.external_fa);

	/* Apply the entry's last-modified timestamp */
	filesys_set_modtime_dos(path, ufi.dosDate >> 16, ufi.dosDate);

	return XARC_OK;
}

/* Function: m_zip_error_description
 * Return a string describing the supplied integer error id.
 *
 * See also: <handler_funcs.error_description>
 */
const xchar* m_zip_error_description(xarc* x, int32_t error_id)
{
	/* If the ID is within the range used by minizip itself, return the error
	 * string from the lookup table <minizip_descriptors>.
	 */
	if (error_id <= -100 && error_id >= -105)
		return minizip_descriptors[-error_id - 100];
	/* Otherwise, call on ZLIB's <gzerror> to get a ZLIB error string or system
	 * (errno/strerror) error string.
	 */
	else
	{
#if XARC_NATIVE_WCHAR
		/* ZLIB only knows about narrow-char strings, so we have to convert to
		 * wide-char and store it temporarily if that's our native format.
		 */
		const char* edesc = gzerror(M_ZIP(x)->file, 0);
		size_t lenl = filesys_localize_char(edesc, -1, 0, 0);
		M_ZIP(x)->localized_error = realloc(M_ZIP(x)->localized_error,
		 sizeof(xchar) * lenl);
		filesys_localize_char(edesc, -1, M_ZIP(x)->localized_error, lenl);
		return M_ZIP(x)->localized_error;
#else
		return gzerror(M_ZIP(x)->file, 0);
#endif
	}
}
