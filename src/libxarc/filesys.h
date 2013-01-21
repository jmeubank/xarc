/* File: libxarc/filesys.h
 * Platform-independent wrappers for the local file system and string
 * localization functions.
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

#ifndef FILESYS_H_INC
#define FILESYS_H_INC

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <xarc.h>


/* Struct: win_filetime
 * Duplicates the WinAPI FILETIME structure, so we can use archives that encode
 * it on other platforms.
 */
typedef struct
{
	uint32_t lo;
	uint32_t hi;
} win_filetime;


/* Section: Functions */


/* Function: filesys_dir_exists
 * Check if a path exists and refers to a directory.
 *
 * Parameters:
 *   dir_path - Check whether this path exists and is a directory.
 *
 * Returns:
 *   0 if the path does not exist or refers to a file rather than a directory;
 *   nonzero if the path exists and refers to a directory
 */
int8_t filesys_dir_exists(const xchar* dir_path);
/* Function: filesys_is_dir_sep
 * Check if a character is a directory separator for the local file system.
 *
 * Parameters:
 *   ch - Check whether this character is a directory separator for this file
 *     system.
 *
 * Returns:
 *   0 if the character is a directory separator; nonzero if it is not.
 */
int8_t filesys_is_dir_sep(xchar ch);
/* Function: filesys_set_attributes_win
 * Set attributes as specified by the WinAPI on a file.
 *
 * Parameters:
 *   path - Set attributes on this file
 *   attr - Set these WinAPI attributes on the file
 */
void filesys_set_attributes_win(const xchar* path, uint32_t attr);
/* Function: filesys_set_attributes_unix
 * Set permissions as specified by the Unix filesystem on a file.
 *
 * Parameters:
 *   path - Set permissions on this file
 *   mode - Set these Unix permissions on the file
 */
void filesys_set_attributes_unix(const xchar* path, int32_t mode);
/* Function: filesys_time_dos
 * Convert an MS-DOS format date & time to an <xarc_time_t>.
 *
 * Parameters:
 *   dosdate - DOS-format packed date
 *   dostime - DOS-format packed time
 *   xtime - <xarc_time_t> to be set from the DOS date & time
 */
void filesys_time_dos(uint16_t dosdate, uint16_t dostime, xarc_time_t* xtime);
/* Function: filesys_time_winft
 * Convert a WinAPI FILETIME to an <xarc_time_t>.
 *
 * Parameters:
 *   wintime - WinAPI FILETIME
 *   xtime - <xarc_time_t> to be set from the FILETIME
 */
void filesys_time_winft(const win_filetime* wintime, xarc_time_t* xtime);
/* Function: filesys_time_unix
 * Convert a Unix timestamp (the number of seconds since the start of 1970) to
 * an <xarc_time_t>.
 *
 * Parameters:
 *   utime - Unix timestamp (the number of seconds since the start of 1970)
 *   xtime - <xarc_time_t> to be set from the timestamp
 */
void filesys_time_unix(uintmax_t utime, xarc_time_t* xtime);
/* Function: filesys_set_modtime_dos
 * Set the modification timestamp on a file from an MS-DOS format date & time.
 *
 * Parameters:
 *   path - The file to set the modification timestamp on
 *   dosdate - DOS-format packed date
 *   dostime - DOS-format packed time
 */
void filesys_set_modtime_dos(const xchar* path, uint16_t dosdate,
 uint16_t dostime);
/* Function: filesys_set_modtime_winft
 * Set the modification timestamp on a file from a WinAPI FILETIME.
 *
 * Parameters:
 *   path - The file to set the modification timestamp on
 *   wintime - The WinAPI FILETIME to use
 */
void filesys_set_modtime_winft(const xchar* path,
 const win_filetime* wintime);
/* Function: filesys_set_modtime_unix
 * Set the modification timestamp on a file from a Unix timestamp (the number of
 * seconds since the start of 1970).
 *
 * Parameters:
 *   path - The file to set the modification timestamp on
 *   utime - Unix timestamp (the number of seconds since the start of 1970)
 */
void filesys_set_modtime_unix(const xchar* path, uintmax_t utime);
#if XARC_NATIVE_WCHAR
/* Function: filesys_localize_char
 * Convert a string in the local 8-bit character format (on Windows, whatever
 * the current ANSI code page is) to UTF-16. Works like the WinAPI
 * MultiByteToWideChar function.
 *
 * Parameters:
 *   char_str - The 8-bit character string to convert
 *   char_len - The number of characters to convert, or -1 to convert up to and
 *     including a terminating null character.
 *   local_out - The buffer to place output in, or NULL to produce no output.
 *   max_out - The maximum number of characters to place in the output buffer,
 *     or 0 to calculate the required number of output characters without
 *     producing any output.
 *
 * Returns:
 *   If max_out is greater than 0, returns the number of characters written to
 *   local_out. If max_out is 0, returns the number of characters that
 *   conversion would produce but does not perform the conversion.
 */
intmax_t filesys_localize_char(const char* char_str, intmax_t char_len,
 xchar* local_out, intmax_t max_out);
/* Function: filesys_narrow_utf16
 * Convert a UTF-16 string to the local 8-bit character format (on Windows,
 * whatever the current ANSI code page is). Works like the WinAPI
 * WideCharToMultiByte function.
 *
 * Parameters:
 *   utf16_str - The UTF-16 character string to convert
 *   utf16_len - The number of characters to convert, or -1 to convert up to and
 *     including a terminating null character.
 *   narrow_out - The buffer to place output in, or NULL to produce no output.
 *   max_out - The maximum number of characters to place in the output buffer,
 *     or 0 to calculate the required number of output characters without
 *     producing any output.
 *
 * Returns:
 *   If max_out is greater than 0, returns the number of characters written to
 *   narrow_out. If max_out is 0, returns the number of characters that
 *   conversion would produce but does not perform the conversion.
 */
intmax_t filesys_narrow_utf16(const uint16_t* utf16_str, intmax_t utf16_len,
 char* narrow_out, intmax_t max_out);
#endif
/* Function: filesys_localize_cp437
 * Convert an ANSI code page 437 (IBM437) char string to the local character
 * format.
 *
 * Parameters:
 *   char_str - The CP437 character string to convert
 *   char_len - The number of characters to convert, or -1 to convert up to and
 *     including a terminating null character.
 *   local_out - The buffer to place output in, or NULL to produce no output.
 *   max_out - The maximum number of characters to place in the output buffer,
 *     or 0 to calculate the required number of output characters without
 *     producing any output.
 *
 * Returns:
 *   If max_out is greater than 0, returns the number of characters written to
 *   local_out. If max_out is 0, returns the number of characters that
 *   conversion would produce but does not perform the conversion.
 */
intmax_t filesys_localize_cp437(const char* char_str, intmax_t char_len,
 xchar* local_out, intmax_t max_out);
/* Function: filesys_localize_utf8
 * Convert a UTF-8 multi-byte string to the local character format.
 *
 * Parameters:
 *   utf8_str - The UTF-8 character string to convert
 *   utf8_len - The number of characters to convert, or -1 to convert up to and
 *     including a terminating null character.
 *   local_out - The buffer to place output in, or NULL to produce no output.
 *   max_out - The maximum number of characters to place in the output buffer,
 *     or 0 to calculate the required number of output characters without
 *     producing any output.
 *
 * Returns:
 *   If max_out is greater than 0, returns the number of characters written to
 *   local_out. If max_out is 0, returns the number of characters that
 *   conversion would produce but does not perform the conversion.
 */
intmax_t filesys_localize_utf8(const char* utf8_str, intmax_t utf8_len,
 xchar* local_out, intmax_t max_out);
#if !XARC_NATIVE_WCHAR
/* Function: filesys_localize_utf16
 * Convert a UTF-16 wide char string to the local character format.
 *
 * Parameters:
 *   utf16_str - The UTF-16 character string to convert
 *   utf16_len - The number of characters to convert, or -1 to convert up to and
 *     including a terminating null character.
 *   local_out - The buffer to place output in, or NULL to produce no output.
 *   max_out - The maximum number of characters to place in the output buffer,
 *     or 0 to calculate the required number of output characters without
 *     producing any output.
 *
 * Returns:
 *   If max_out is greater than 0, returns the number of characters written to
 *   local_out. If max_out is 0, returns the number of characters that
 *   conversion would produce but does not perform the conversion.
 */
intmax_t filesys_localize_utf16(const uint16_t* utf16_str, intmax_t utf16_len,
 xchar* local_out, intmax_t max_out);
#endif
/* Function: filesys_read_open
 * Opens a file for reading, returning a file descriptor.
 *
 * Parameters:
 *   path - Path of the file to open
 *
 * Returns:
 *   A file descriptor if opened successfully, or -1.
 */
int filesys_read_open(const xchar* path);
/* Function: filesys_ensure_writable
 * Tries to set appropriate permissions on a file so that it can be opened
 * for writing.
 *
 * Parameters:
 *   path - Path of the file to set permissions on
 */
void filesys_ensure_writable(const xchar* path);
/* Function: filesys_mkdir
 * Creates an intermediate directory for an entry being extracted.
 *
 * Parameters:
 *   path - Path of the directory to create
 *
 * Returns:
 *   0 if successful; -1 (and sets errno) otherwise.
 */
int filesys_mkdir(const xchar* path);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // FILESYS_H_INC
