/** \file mod_7z.c
 *
 * Created: JohnE, 2010-08-09
 */


#include <7z.h>
#include <7zFile.h>
#include <7zCrc.h>
#include <Util/7z/7zAlloc.h>
#include <inttypes.h>
#include <malloc.h>
#include <string.h>
#include "xarc_impl.h"
#include "filesys.h"


typedef struct
{
	CFileInStream instream;
	CLookToRead lookstream;
	CSzArEx db;
	uint32_t entry;
	xchar* entry_path;
	UInt32 block_index; /* it can have any value before first call (if outBuffer = 0) */
	Byte *out_buffer; /* it must be 0 before first call for each new archive. */
	size_t out_buffer_size;  /* it can have any value before first call (if outBuffer = 0) */
} m_7z_extra;
#define M_7Z(x) ((m_7z_extra*)((void*)x + sizeof(struct _xarc)))


xarc_result_t m_7z_open(xarc* x, const xchar* file, uint8_t type);
xarc_result_t m_7z_close(xarc* x);
xarc_result_t m_7z_next_item(xarc* x);
xarc_result_t m_7z_item_get_info(xarc* x, xarc_item_info* info);
xarc_result_t m_7z_item_extract(xarc* x, FILE* to, size_t* written);
xarc_result_t m_7z_item_set_props(xarc* x, const xchar* path);
const xchar* m_7z_error_description(xarc* x, int32_t error_id);


XARC_DEFINE_MODULE(mod_7z, m_7z_open, sizeof(m_7z_extra))


handler_funcs sz_funcs = {
	m_7z_close,
	m_7z_next_item,
	m_7z_item_get_info,
	m_7z_item_extract,
	m_7z_item_set_props,
	m_7z_error_description
};


ISzAlloc g_alloc = { SzAlloc, SzFree };
ISzAlloc g_alloc_temp = { SzAllocTemp, SzFreeTemp };

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


static WRes InFile_OpenX(CSzFile *p, const xchar *name)
{
#if XARC_NATIVE_WCHAR
	return InFile_OpenW(p, name);
#else
	return InFile_Open(p, name);
#endif
}


xarc_result_t m_7z_open(xarc* x, const xchar* file, uint8_t type __attribute__((unused)))
{
	memset(M_7Z(x), 0, sizeof(m_7z_extra));
	X_BASE(x)->impl = &sz_funcs;

	if (InFile_OpenX(&M_7Z(x)->instream.file, file))
	{
		return xarc_set_error(x, XARC_ERR_NOT_VALID_ARCHIVE, 0,
		 XC("Failed to open '%s' for reading"), file);
	}
	FileInStream_CreateVTable(&M_7Z(x)->instream);

	LookToRead_CreateVTable(&M_7Z(x)->lookstream, False);
	M_7Z(x)->lookstream.realStream = &M_7Z(x)->instream.s;
	LookToRead_Init(&M_7Z(x)->lookstream);

	CrcGenerateTable();

	SzArEx_Init(&M_7Z(x)->db);
	if (SzArEx_Open(&M_7Z(x)->db, &M_7Z(x)->lookstream.s, &g_alloc,
	 &g_alloc_temp) != SZ_OK)
	{
		File_Close(&M_7Z(x)->instream.file);
		return xarc_set_error(x, XARC_ERR_NOT_VALID_ARCHIVE, 0,
		 XC("Failed to open '%s' as a 7z archive"), file);
	}

	return XARC_OK;
}

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

xarc_result_t m_7z_next_item(xarc* x)
{
	if (M_7Z(x)->entry + 1 >= M_7Z(x)->db.db.NumFiles)
		return xarc_set_error(x, XARC_NO_MORE_ITEMS, 0, XC("End of 7z archive"));
	++(M_7Z(x)->entry);
	if (M_7Z(x)->entry_path)
	{
		free(M_7Z(x)->entry_path);
		M_7Z(x)->entry_path = 0;
	}
	return XARC_OK;
}

xarc_result_t m_7z_item_get_info(xarc* x, xarc_item_info* info)
{
	if (!M_7Z(x)->entry_path)
	{
		size_t len16 = SzArEx_GetFileNameUtf16(&M_7Z(x)->db, M_7Z(x)->entry, 0);
#if XARC_NATIVE_WCHAR
		M_7Z(x)->entry_path = malloc(sizeof(uint16_t) * len16);
		SzArEx_GetFileNameUtf16(&M_7Z(x)->db, M_7Z(x)->entry,
		 M_7Z(x)->entry_path);
#else
		uint16_t* path16 = malloc(sizeof(uint16_t) * len16);
		SzArEx_GetFileNameUtf16(&M_7Z(x)->db, M_7Z(x)->entry, path16);
		size_t lenx = filesys_localize_utf16(path16, len16, 0, 0);
		M_7Z(x)->entry_path = malloc(sizeof(xchar) * lenx);
		filesys_localize_utf16(path16, len16, M_7Z(x)->entry_path, lenx);
		free(path16);
#endif
	}
	info->path.native = M_7Z(x)->entry_path;

	if (M_7Z(x)->db.db.Files[M_7Z(x)->entry].IsDir)
		info->properties = XARC_PROP_DIR;
	else
		info->properties = 0;

	if (M_7Z(x)->db.db.Files[M_7Z(x)->entry].MTimeDefined)
	{
		filesys_time_winft((const win_filetime*)&M_7Z(x)->db.db.Files[M_7Z(x)->entry].MTime,
		 &info->mod_time);
	}

	return XARC_OK;
}

xarc_result_t m_7z_item_extract(xarc* x, FILE* to, size_t* written)
{
	size_t offset, out_processed;
	SRes res = SzArEx_Extract(&M_7Z(x)->db, &M_7Z(x)->lookstream.s,
	 M_7Z(x)->entry, &M_7Z(x)->block_index, &M_7Z(x)->out_buffer,
	 &M_7Z(x)->out_buffer_size, &offset, &out_processed, &g_alloc,
	 &g_alloc_temp);
	if (res != SZ_OK)
	{
		return xarc_set_error(x, XARC_MODULE_ERROR, res,
		 XC("7zlib failed to unpack file"));
	}
	int wr = fwrite(M_7Z(x)->out_buffer + offset, 1, out_processed, to);
	if (wr <= 0 || (size_t)wr != out_processed)
	{
		return xarc_set_error_filesys(x, XC("Failed to write %"PRIuMAX" bytes"),
		 out_processed);
	}
	*written = out_processed;
	return XARC_OK;
}

xarc_result_t m_7z_item_set_props(xarc* x, const xchar* path)
{
	if (M_7Z(x)->db.db.Files[M_7Z(x)->entry].AttribDefined)
	{
		filesys_set_attributes_win(path,
		 M_7Z(x)->db.db.Files[M_7Z(x)->entry].Attrib);
	}
	if (M_7Z(x)->db.db.Files[M_7Z(x)->entry].MTimeDefined)
	{
		filesys_set_modtime_winft(path,
		 (const win_filetime*)&M_7Z(x)->db.db.Files[M_7Z(x)->entry].MTime);
	}
	return XARC_OK;
}

const xchar* m_7z_error_description(xarc* x __attribute__((unused)),
 int32_t error_id)
{
	if (error_id < 1 || error_id > 17)
		return XC("[undefined]");
	return sz_error_names[error_id - 1];
}
