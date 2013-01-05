/** \file mod_zip.c
 *
 * Created: JohnE, 2010-07-19
 */


#include <string.h>
#include "xarc_impl.h"
#include "filesys.h"
#include "unzip.h"
#if defined(_WIN32) || defined(_WIN64)
#include "iowin32.h"
#endif


typedef struct
{
	unzFile file;
	xchar* item_path;
#if XARC_NATIVE_WCHAR
	wchar_t* localized_error;
#endif
} m_zip_extra;
#define M_ZIP(x) ((m_zip_extra*)((void*)x + sizeof(struct _xarc)))


xarc_result_t m_zip_open(xarc* x, const xchar* file, uint8_t type);
xarc_result_t m_zip_close(xarc* x);
xarc_result_t m_zip_next_item(xarc* x);
xarc_result_t m_zip_item_get_info(xarc* x, xarc_item_info* info);
xarc_result_t m_zip_item_extract(xarc* x, FILE* to, size_t* written);
xarc_result_t m_zip_item_set_props(xarc* x, const xchar* path);
const xchar* m_zip_error_description(xarc* x, int32_t error_id);


XARC_DEFINE_MODULE(mod_minizip, m_zip_open, sizeof(m_zip_extra))


handler_funcs zip_funcs = {
	m_zip_close,
	m_zip_next_item,
	m_zip_item_get_info,
	m_zip_item_extract,
	m_zip_item_set_props,
	m_zip_error_description
};

const xchar* minizip_descriptors[] = {
	XC("No more items in zip file"), /* UNZ_END_OF_LIST_OF_FILE (-100) */
	XC("[invalid minizip error]"),
	XC("Parameter error calling minizip function"), /* UNZ_PARAMERROR (-102) */
	XC("Corrupt or invalid zip archive"), /* UNZ_BADZIPFILE (-103) */
	XC("Internal error in minizip"), /* UNZ_INTERNALERROR (-104) */
	XC("CRC error in zip archive") /* UNZ_CRCERROR (-105) */
};

static xarc_result_t set_error_zip(xarc* x, int mzerror)
{
	return (mzerror == UNZ_ERRNO) ?
	 xarc_set_error_filesys(x, 0) :
	 xarc_set_error(x, XARC_MODULE_ERROR, mzerror, 0);
}


xarc_result_t m_zip_open(xarc* x, const xchar* file, uint8_t type __attribute__((unused)))
{
	memset(M_ZIP(x), 0, sizeof(m_zip_extra));
	X_BASE(x)->impl = &zip_funcs;

	zlib_filefunc64_def zfuncs;
#if defined(_WIN32) || defined(_WIN64)
	fill_win32_filefunc64(&zfuncs);
#else
	fill_fopen64_filefunc(&zfuncs);
#endif
	unzFile f = unzOpen2_64((const char*)file, &zfuncs);
	if (!f)
	{
		return xarc_set_error(x, XARC_ERR_NOT_VALID_ARCHIVE, 0,
		 XC("minizip failed to open '%s' as a ZIP archive"), file);
	}
	M_ZIP(x)->file = f;

	return XARC_OK;
}

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

xarc_result_t m_zip_next_item(xarc* x)
{
	int ret = unzGoToNextFile(M_ZIP(x)->file);
	if (ret == UNZ_END_OF_LIST_OF_FILE)
		return xarc_set_error(x, XARC_NO_MORE_ITEMS, ret, 0);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);
	return XARC_OK;
}

xarc_result_t m_zip_item_get_info(xarc* x, xarc_item_info* info)
{
	unz_file_info ufi;
	int ret = unzGetCurrentFileInfo(M_ZIP(x)->file, &ufi, 0, 0, 0, 0, 0, 0);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);

	char* path8 = malloc(ufi.size_filename + 1);
	ret = unzGetCurrentFileInfo(M_ZIP(x)->file, &ufi, path8,
	 ufi.size_filename + 1, 0, 0, 0, 0);
	if (ret != UNZ_OK)
	{
		free(M_ZIP(x)->item_path);
		M_ZIP(x)->item_path = 0;
		return set_error_zip(x, ret);
	}
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
		filesys_localize_cp437(path8, ufi.size_filename + 1, M_ZIP(x)->item_path,
		 lenl);
	}
	free(path8);

	memset(info, 0, sizeof(xarc_item_info));

	info->path.native = M_ZIP(x)->item_path;

	if (M_ZIP(x)->item_path[ufi.size_filename - 1] == '/'
	 || ((ufi.version >> 8 == 0 || ufi.version >> 8 == 10) && (ufi.external_fa & 0x0010)))
		info->properties |= XARC_PROP_DIR;

	filesys_time_dos(ufi.dosDate >> 16, ufi.dosDate, &info->mod_time);

	return XARC_OK;
}

xarc_result_t m_zip_item_extract(xarc* x, FILE* to, size_t* written)
{
	int ret = unzOpenCurrentFile(M_ZIP(x)->file);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);

	*written = 0;
	char buf[4096];
	while (1)
	{
		ret = unzReadCurrentFile(M_ZIP(x)->file, buf, 4096);
		if (ret < 0)
			return set_error_zip(x, ret);
		if (ret == 0)
			break;
		size_t wr = fwrite(buf, 1, ret, to);
		if (wr > 0)
			*written += wr;
		if (wr != (size_t)ret)
			return xarc_set_error_filesys(x, 0);
	}

	ret = unzCloseCurrentFile(M_ZIP(x)->file);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);

	return XARC_OK;
}

xarc_result_t m_zip_item_set_props(xarc* x, const xchar* path)
{
	unz_file_info ufi;
	int ret = unzGetCurrentFileInfo(M_ZIP(x)->file, &ufi, 0, 0, 0, 0, 0, 0);
	if (ret != UNZ_OK)
		return set_error_zip(x, ret);

	if (ufi.version >> 8 == 0 || ufi.version >> 8 == 10)
		filesys_set_attributes_win(path, ufi.external_fa);

	filesys_set_modtime_dos(path, ufi.dosDate >> 16, ufi.dosDate);

	return XARC_OK;
}

const xchar* m_zip_error_description(xarc* x, int32_t error_id)
{
	if (error_id <= -100 && error_id >= -105)
		return minizip_descriptors[-error_id - 100];
	else
	{
#if XARC_NATIVE_WCHAR
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
