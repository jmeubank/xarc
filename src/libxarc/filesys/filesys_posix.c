/** \file filesys_posix.c
 *
 * Created: JohnE, 2010-09-09
 */


#include "filesys.h"

#include <wchar.h>
#include <string.h>
#include <time.h>
#include <utime.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "xchar.h"


static const uintmax_t SECONDS_1601_1970 = (((uintmax_t)369U * 365U) + 89U)
 * 24U * 60U * 60U;

static intmax_t do_nothing_localize(const char* char_in, intmax_t in_len,
 char* char_out, intmax_t out_len)
{
	if (in_len < 0)
		in_len = strlen(char_in) + 1;
	if (char_out)
	{
		if (in_len > out_len)
			in_len = out_len;
		memcpy(char_out, char_in, in_len);
	}
	return in_len;
}


int8_t filesys_dir_exists(const xchar* dir_path)
{
	struct stat st;
	if (stat(dir_path, &st) != 0)
		return 0;
	return (S_ISDIR(st.st_mode)) ? 1 : 0;
}

int8_t filesys_is_dir_sep(xchar ch)
{
	return (ch == XC('/'));
}

void filesys_set_attributes_win(const xchar* path __attribute__((unused)), uint32_t attr __attribute__((unused)))
{
	chmod(path, 0774);
}

void filesys_set_attributes_unix(const xchar* path, int32_t mode)
{
	chmod(path, mode);
}

void filesys_time_dos(uint16_t dosdate, uint16_t dostime, xarc_time_t* xtime)
{
	struct tm local;
	local.tm_year = (uintmax_t)(dosdate >> 9) + 80;
	local.tm_mon = (uintmax_t)((dosdate >> 5) & 0xfU) - 1;
	local.tm_mday = (uintmax_t)(dosdate & 0x1fU);
	local.tm_hour = (uintmax_t)(dostime >> 11);
	local.tm_min = (uintmax_t)(dostime >> 5) & 0x3fU;
	local.tm_sec = (uintmax_t)(dostime & 0x1fU) * 2;
	local.tm_isdst = -1;
	xtime->seconds = mktime(&local);
	xtime->nano = 0;
}

void filesys_time_winft(const win_filetime* wintime, xarc_time_t* xtime)
{
#ifdef UINT64_MAX
	uint64_t t64 = ((uint64_t)wintime->hi << 32) + (uint64_t)wintime->lo;
	xtime->seconds = t64 / __UINT64_C(10000000) - SECONDS_1601_1970;
	xtime->nano = t64 % (__UINT64_C(10000000)) * 100;
#else
	wintime = wintime;
	xtime->seconds = 0;
	xtime->nano = 0;
#endif
}

void filesys_time_unix(uintmax_t utime, xarc_time_t* xtime)
{
	xtime->seconds = utime;
	xtime->nano = 0;
}

void filesys_set_modtime_dos(const xchar* path, uint16_t dosdate,
 uint16_t dostime)
{
	struct tm local;
	local.tm_year = (uintmax_t)(dosdate >> 9) + 80;
	local.tm_mon = (uintmax_t)((dosdate >> 5) & 0xfU) - 1;
	local.tm_mday = (uintmax_t)(dosdate & 0x1fU);
	local.tm_hour = (uintmax_t)(dostime >> 11);
	local.tm_min = (uintmax_t)(dostime >> 5) & 0x3fU;
	local.tm_sec = (uintmax_t)(dostime & 0x1fU) * 2;
	local.tm_isdst = -1;
	time_t t = mktime(&local);
	struct utimbuf utb;
	utb.actime = t;
	utb.modtime = t;
	utime(path, &utb);
}

void filesys_set_modtime_winft(const xchar* path, const win_filetime* wintime)
{
#ifdef UINT64_MAX
	time_t sec = (((uint64_t)wintime->hi << 32) + (uint64_t)wintime->lo)
	 / __UINT64_C(10000000) - SECONDS_1601_1970;
	struct utimbuf utb;
	utb.actime = sec;
	utb.modtime = sec;
	utime(path, &utb);
#else
	path = path;
	wintime = wintime;
#endif
}

void filesys_set_modtime_unix(const xchar* path, uintmax_t unix_time)
{
	struct utimbuf utb;
	utb.actime = unix_time;
	utb.modtime = unix_time;
	utime(path, &utb);
}

intmax_t filesys_localize_cp437(const char* char_str, intmax_t char_len,
 xchar* local_out, intmax_t max_out)
{
	return do_nothing_localize(char_str, char_len, local_out, max_out);
}

intmax_t filesys_localize_utf8(const char* utf8_str, intmax_t utf8_len,
 xchar* local_out, intmax_t max_out)
{
	return do_nothing_localize(utf8_str, utf8_len, local_out, max_out);
}

intmax_t filesys_localize_utf16(const uint16_t* utf16_str, intmax_t utf16_len,
 xchar* local_out, intmax_t max_out)
{
	if (utf16_len < 0)
		utf16_len = wcslen((const wchar_t*)utf16_str) + 1;
	if (local_out)
	{
		if (utf16_len > max_out)
			utf16_len = max_out;
		intmax_t i;
		for (i = 0; i < utf16_len; ++i)
			local_out[i] = utf16_str[i];
	}
	return utf16_len;
}

int filesys_read_open(const xchar* path)
{
	return open(path, O_RDONLY);
}

void filesys_ensure_writable(const xchar* path)
{
	chmod(path, S_IWUSR);
}

int filesys_mkdir(const xchar* path)
{
	return mkdir(path, 0664);
}

