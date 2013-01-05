/** \file filesys_win32.c
 *
 * Created: JohnE, 2010-08-02
 */


#include "filesys.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>
#include <io.h>
#include <tchar.h>
#include <malloc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>
#include <xarc/xchar.h>


static const uintmax_t SECONDS_1601_1970 = (((uintmax_t)369U * 365U) + 89U)
 * 24U * 60U * 60U;


int8_t filesys_dir_exists(const xchar* dir_path)
{
	DWORD ret = GetFileAttributes(dir_path);
	return (ret == INVALID_FILE_ATTRIBUTES) ? 0 : (ret & FILE_ATTRIBUTE_DIRECTORY);
}

int8_t filesys_is_dir_sep(xchar ch)
{
	return (ch == XC('/') || ch == XC('\\'));
}

void filesys_set_attributes_win(const xchar* path, uint32_t attr)
{
	SetFileAttributes(path, attr);
}

void filesys_set_attributes_unix(const xchar* path, int32_t mode)
{
	_tchmod(path, mode);
}

void filesys_time_dos(uint16_t dosdate, uint16_t dostime, xarc_time_t* xtime)
{
	FILETIME ft;
	if (DosDateTimeToFileTime(dosdate, dostime, &ft))
	{
		uint64_t t64 = ((uint64_t)ft.dwHighDateTime << 32) + (uint64_t)ft.dwLowDateTime;
		xtime->seconds = t64 / __UINT64_C(10000000) - SECONDS_1601_1970;
		xtime->nano = (t64 % __UINT64_C(10000000)) * 100;
	}
}

void filesys_time_winft(const win_filetime* wintime, xarc_time_t* xtime)
{
	uint64_t t64 = ((uint64_t)wintime->hi << 32) + (uint64_t)wintime->lo;
	xtime->seconds = t64 / __UINT64_C(10000000) - SECONDS_1601_1970;
	xtime->nano = (t64 % __UINT64_C(10000000)) * 100;
}

void filesys_time_unix(uintmax_t utime, xarc_time_t* xtime)
{
	xtime->seconds = utime;
	xtime->nano = 0;
}

void filesys_set_modtime_dos(const xchar* path, uint16_t dosdate,
 uint16_t dostime)
{
	FILETIME ft;
	if (DosDateTimeToFileTime(dosdate, dostime, &ft))
		filesys_set_modtime_winft(path, (const win_filetime*)&ft);
}

void filesys_set_modtime_winft(const xchar* path, const win_filetime* wintime)
{
	HANDLE h = CreateFile(path, FILE_WRITE_ATTRIBUTES, 0, 0, OPEN_EXISTING,
	 FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		SetFileTime(h, 0, 0, (const FILETIME*)wintime);
		CloseHandle(h);
	}
}

void filesys_set_modtime_unix(const xchar* path, uintmax_t utime)
{
	uint64_t ft64 = ((uint64_t)utime + SECONDS_1601_1970) * __UINT64_C(10000000);
	FILETIME ft;
	ft.dwLowDateTime = ft64;
    ft.dwHighDateTime = ft64 >> 32;
    filesys_set_modtime_winft(path, (const win_filetime*)&ft);
}

#if XARC_NATIVE_WCHAR
intmax_t filesys_localize_char(const char* char_str, intmax_t char_len,
 xchar* local_out, intmax_t max_out)
{
	return MultiByteToWideChar(CP_ACP, 0, char_str, char_len, local_out,
	 max_out);
}

intmax_t filesys_narrow_utf16(const uint16_t* utf16_str, intmax_t utf16_len,
 char* narrow_out, intmax_t max_out)
{
	return WideCharToMultiByte(CP_ACP, 0, utf16_str, utf16_len, narrow_out,
	 max_out, 0, 0);
}
#endif /* defined XARC_NATIVE_WCHAR */

intmax_t filesys_localize_cp437(const char* char_str, intmax_t char_len,
 xchar* local_out, intmax_t max_out)
{
#if XARC_NATIVE_WCHAR
	return MultiByteToWideChar(437, 0, char_str, char_len, local_out, max_out);
#else
	intmax_t len16 = MultiByteToWideChar(437, 0, char_str, char_len, 0, 0);
	wchar_t* str16 = malloc(sizeof(wchar_t) * len16);
	MultiByteToWideChar(437, 0, char_str, char_len, str16, len16);
	intmax_t ret = WideCharToMultiByte(CP_ACP, 0, str16, len16, local_out,
	 max_out, 0, 0);
	free(str16);
	return ret;
#endif
}

intmax_t filesys_localize_utf8(const char* utf8_str, intmax_t utf8_len,
 xchar* local_out, intmax_t max_out)
{
#if XARC_NATIVE_WCHAR
	return MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, local_out,
	 max_out);
#else
	intmax_t len16 = MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, 0, 0);
	wchar_t* str16 = malloc(sizeof(wchar_t) * len16);
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, str16, len16);
	intmax_t ret = WideCharToMultiByte(CP_ACP, 0, str16, len16, local_out,
	 max_out, 0, 0);
	free(str16);
	return ret;
#endif
}

#if !XARC_NATIVE_WCHAR
intmax_t filesys_localize_utf16(const uint16_t* utf16_str, intmax_t utf16_len,
 xchar* local_out, intmax_t max_out)
{
	return WideCharToMultiByte(CP_ACP, 0, utf16_str, utf16_len, local_out,
	 max_out, 0, 0);
}
#endif

int filesys_read_open(const xchar* path)
{
#if XARC_NATIVE_WCHAR
	return _wopen(path, _O_RDONLY | _O_BINARY, 0);
#else
	return _open(path, _O_RDONLY | _O_BINARY, 0);
#endif
}

void filesys_ensure_writable(const xchar* path)
{
#if XARC_NATIVE_WCHAR
	_wchmod(path, _S_IWRITE);
#else
	_chmod(path, _S_IWRITE);
#endif
}

int filesys_mkdir(const xchar* path)
{
#if XARC_NATIVE_WCHAR
	return _wmkdir(path);
#else
	return _mkdir(path);
#endif
}
