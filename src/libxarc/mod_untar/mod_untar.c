/* File: libxarc/mod_untar/mod_untar.c
 * TAR archive module.
 *
 * This implementation is partly based on contrib/untgz/untgz.c from the
 * official 'zlib' sources. The code has been almost entirely rewritten in order
 * to fit with XARC's data flow; the only remaining verbatim portions are the
 * TAR header struct, the 'getoct' function ('untgz_getoct'), and the macro
 * constants.
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


#include <malloc.h>
#include <string.h>
#include "filesys.h"
#include "xarc_decompress.h"
#include "xarc_impl.h"


/* Struct: m_untar_extra
 * typedef struct {...} m_untar_extra - Data specific to the TAR archive
 * module.
 */
typedef struct
{
	/* Field: decomp
	 * Decompressor implementation for this 2-phase archive type.
	 *
	 * See: <xarc_decompress_impl>
	 */
	xarc_decompress_impl* decomp;
	/* Field: entry_properties
	 * Bitfield holding the current entry's platform-independent flags.
	 *
	 * See: <XARC entry properties>
	 */
	uint8_t entry_properties;
	/* Field: entry_bytes_remaining
	 * The size of the current entry if it's a file
	 */
	size_t entry_bytes_remaining;
	/* Field: entry_path
	 * The relative path of the current entry.
	 *
	 * Should always be usable, as we store it when reading through the current
	 * entry's headers.
	 */
	xchar* entry_path;
	/* Field: entry_time
	 * The last-modified timestamp of the current entry.
	 */
	int32_t entry_time;
	/* Field: entry_mode
	 * The Unix-style filesystem attributes (permissions) of the current entry.
	 */
	int32_t entry_mode;
} m_untar_extra;
#define M_UNTAR(x) ((m_untar_extra*)((void*)x + sizeof(struct _xarc)))


/* Section: Global Data */


/* values used in typeflag field */
#define REGTYPE  '0'            /* regular file */
#define AREGTYPE '\0'           /* regular file */
#define LNKTYPE  '1'            /* link */
#define SYMTYPE  '2'            /* reserved */
#define CHRTYPE  '3'            /* character special */
#define BLKTYPE  '4'            /* block special */
#define DIRTYPE  '5'            /* directory */
#define FIFOTYPE '6'            /* FIFO special */
#define CONTTYPE '7'            /* reserved */
/* GNU tar extensions */
#define GNUTYPE_DUMPDIR  'D'    /* file names from dumped directory */
#define GNUTYPE_LONGLINK 'K'    /* long link name */
#define GNUTYPE_LONGNAME 'L'    /* long file name */
#define GNUTYPE_MULTIVOL 'M'    /* continuation of file from another volume */
#define GNUTYPE_NAMES    'N'    /* file name that does not fit into main hdr */
#define GNUTYPE_SPARSE   'S'    /* sparse file */
#define GNUTYPE_VOLHDR   'V'    /* tape/volume header */
/* tar header */
#define BLOCKSIZE     512
#define SHORTNAMESIZE 100

/* The TAR header has a constant size (i.e. no variable-length fields); we can
 * read an entire header object in with a single read op. */
struct tar_header
{                               /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
};


xarc_result_t m_untar_open(xarc* x, const xchar* file, uint8_t type);
xarc_result_t m_untar_close(xarc* x);
xarc_result_t m_untar_next_item(xarc* x);
xarc_result_t m_untar_item_get_info(xarc* x, xarc_item_info* info);
xarc_result_t m_untar_item_extract(xarc* x, FILE* to, size_t* written);
xarc_result_t m_untar_item_set_props(xarc* x, const xchar* path);
const xchar* m_untar_error_description(xarc* x, int32_t error_id);


/* Link m_untar_open as the opener function for the mod_untar archive module. */
XARC_DEFINE_MODULE(mod_untar, m_untar_open, sizeof(m_untar_extra))


/* TAR error codes */
#define M_UNTAR_TRUNCATED	-1
#define M_UNTAR_CORRUPT		-2


/* Variable: untar_funcs
 * Implements the <handler_funcs> interface for mod_untar
 */
handler_funcs untar_funcs = {
	m_untar_close,
	m_untar_next_item,
	m_untar_item_get_info,
	m_untar_item_extract,
	m_untar_item_set_props,
	m_untar_error_description
};


/* Section: Static Functions */


/* Function: untgz_getoct
 * Convert a TAR octal string to native integer
 *
 * Integers are stored in octal string form in a TAR archive; this converts them
 * to native integer form.
 */
static int32_t untgz_getoct(char* p, int32_t width)
{
	int32_t result = 0;
	char c;
	while (width--)
	{
		c = *p++;
		if (c == 0)
			break;
		if (c == ' ')
			continue;
		if (c < '0' || c > '7')
			return -1;
		result = result * 8 + (c - '0');
	}
	return result;
}

/* Function: read_tar_headers
 * Read the TAR headers for an entry
 *
 * An entry in a TAR archive consists of one or more 512-byte header blocks,
 * optionally followed by data blocks if the entry is a file. This function
 * reads through the header blocks to obtain all of the entry's metadata.
 */
static xarc_result_t read_tar_headers(xarc* x)
{
	/* Free the path stored for the previous entry, if there was one. */
	if (M_UNTAR(x)->entry_path)
	{
		free(M_UNTAR(x)->entry_path);
		M_UNTAR(x)->entry_path = 0;
	}
	/* Clear the previous entry's properties, if there was one. */
	M_UNTAR(x)->entry_properties = 0;

	/* Keep reading header blocks until we know that the next block is either
	 * data for the current entry, or a new entry. */
	size_t read_count;
	struct tar_header th;
	while (1)
	{
		/* A TAR block is BLOCKSIZE bytes (512 currently). Try to get 1 block
		 * from the decompressor.
		 */
		read_count = BLOCKSIZE;
		xarc_result_t ret = M_UNTAR(x)->decomp->read(x, M_UNTAR(x)->decomp, &th,
		 &read_count);
		/* If the return is not okay and not EOF, the decompressor has already
		 * set the appropriate error state in the <xarc> object, so just return
		 * the decompressor's error code.
		 */
		if (ret != XARC_OK && ret != XARC_DECOMPRESS_EOF)
			return ret;
		/* If no data was read, we've reached the end of the archive. */
		if (read_count == 0)
		{
			return xarc_set_error(x, XARC_NO_MORE_ITEMS, 0,
			 XC("EOF reached on TAR archive"));
		}
		/* If we didn't read exactly BLOCKSIZE bytes, the TAR archive is
		 * invalid (truncated); all TAR archives are written in multiples of
		 * BLOCKSIZE.
		 */
		if (read_count != BLOCKSIZE)
		{
			return xarc_set_error(x, XARC_MODULE_ERROR, M_UNTAR_TRUNCATED,
			 XC("Unable to read full tar header block"));
		}

		/* If the header has an empty string in the 'name' field, it signifies
		 * the end of the archive.
		 */
		if (th.name[0] == 0)
		{
			return xarc_set_error(x, XARC_NO_MORE_ITEMS, 0,
			 XC("EOF reached on TAR archive"));
		}

		/* Retrieve the entry's permissions and last-modified timestamp for
		 * every header block. Not sure if this is correct...
		 */
		M_UNTAR(x)->entry_mode = untgz_getoct(th.mode, 8);
		M_UNTAR(x)->entry_time = untgz_getoct(th.mtime, 12);
		/* untgz_getoct returns -1 if the string didn't represent a valid
		 * integer.
		 */
		if (M_UNTAR(x)->entry_mode == -1 || M_UNTAR(x)->entry_time == -1)
		{
			return xarc_set_error(x, XARC_MODULE_ERROR, M_UNTAR_CORRUPT,
			 XC("Invalid values in tar header"));
		}

		/* Retrieve the entry's relative path from within the constant-length
		 * 'name' field, *unless* the entry type is GNUTYPE_LONGLINK or
		 * GNUTYPE_LONGNAME. In that case, the entry's path is too long to fit
		 * in the field, and it is instead stored right after this header block.
		 */
		if (th.typeflag != GNUTYPE_LONGLINK && th.typeflag != GNUTYPE_LONGNAME
		 && !M_UNTAR(x)->entry_path)
		{
			/* SHORTNAMESIZE is the constant length of the 'name' field. */
			char path8[SHORTNAMESIZE + 1];
			strncpy(path8, th.name, SHORTNAMESIZE);
			/* strncpy will copy the terminating NUL if the string is *shorter*
			 * than SHORTNAMESIZE, but if not we have to tack it on ourselves.
			 */
			path8[SHORTNAMESIZE] = '\0';
			/* The TAR format stores strings in UTF-8 format. Need to convert
			 * to native format. */
			size_t lenl = filesys_localize_utf8(path8, -1, 0, 0);
			M_UNTAR(x)->entry_path = malloc(sizeof(xchar) * lenl);
			filesys_localize_utf8(path8, -1, M_UNTAR(x)->entry_path, lenl);
		}

		/* The type of entry in this header determines whether we are done
		 * reading headers or need to keep going.
		 */
		switch (th.typeflag)
		{
			/* DIRTYPE - This entry is a directory, and we are done. */
			case DIRTYPE:
				M_UNTAR(x)->entry_properties |= XARC_PROP_DIR;
				M_UNTAR(x)->entry_bytes_remaining = 0;
				return XARC_OK;
			/* REGTYPE/AREGTYPE - This entry is a file, and we are ready to read
			 * the file data.
			 */
			case REGTYPE:
			case AREGTYPE:
				{
					/* The 'size' field of the header contains the size of the
					 * file, in bytes.
					 */
					int32_t fsize = untgz_getoct(th.size, 12);
					if (fsize < 0)
					{
						return xarc_set_error(x, XARC_MODULE_ERROR,
						 M_UNTAR_CORRUPT,
						 XC("Invalid value for size of entry"));
					}
					M_UNTAR(x)->entry_bytes_remaining = fsize;
					return XARC_OK;
				}
			/* GNUTYPE_LONGLINK/GNUTYPE_LONGNAME - This entry has a path that is
			 * too long for the 'name' field; instead, the path is stored in one
			 * or more of the next blocks, and after that a normal header.
			 */
			case GNUTYPE_LONGLINK:
			case GNUTYPE_LONGNAME:
				{
					/* The 'size' field of this header contains the length of
					 * the path string, in bytes.
					 */
					int32_t name_len = untgz_getoct(th.size, 12);
					/* untgz_getoct returns -1 if the string didn't represent a
					 * valid integer.
					 */
					if (name_len < 1)
					{
						return xarc_set_error(x, XARC_MODULE_ERROR,
						 M_UNTAR_CORRUPT,
						 XC("Invalid value for length of long name"));
					}

					/* Read in the long path */
					char* path8 = malloc(name_len + 1);
					read_count = name_len;
					ret = M_UNTAR(x)->decomp->read(x, M_UNTAR(x)->decomp, path8,
					 &read_count);
					/* If the return is not okay and not EOF, the decompressor
					 * has already set the appropriate error state in the <xarc>
					 * object, so just return the decompressor's error code.
					 */
					if (ret != XARC_OK && ret != XARC_DECOMPRESS_EOF)
						return ret;
					/* However, if we encountered EOF then the TAR is definitely
					 * not complete; we still needed at least a standard header
					 * block.
					 */
					if (read_count != (size_t)name_len)
					{
						return xarc_set_error(x, XARC_MODULE_ERROR,
						 M_UNTAR_TRUNCATED,
						 XC("Unexpected EOF while reading long name"));
					}
					/* TAR long names don't come with terminating NULs, so we
					 * have to add it.
					 */
					path8[name_len] = '\0';

					/* TAR strings are UTF-8; convert to native format */
					size_t lenl = filesys_localize_utf8(path8, name_len + 1, 0,
					 0);
					M_UNTAR(x)->entry_path = realloc(M_UNTAR(x)->entry_path,
					 sizeof(xchar) * lenl);
					filesys_localize_utf8(path8, name_len + 1,
					 M_UNTAR(x)->entry_path, lenl);
					free(path8);

					/* Now we have to read and discard empty data to make up the
					 * rest of the TAR block
					 */
					read_count = BLOCKSIZE - (name_len % BLOCKSIZE);
					if (read_count > 0)
					{
						/* Try to get the rest of the data from the decompressor
						 */
						ret = M_UNTAR(x)->decomp->read(x, M_UNTAR(x)->decomp,
						 &th, &read_count);
						/* If unsuccessful, the decompressor has already set the
						 * error state
						 */
						if (ret != XARC_OK && ret != XARC_DECOMPRESS_EOF)
							return ret;
						if (read_count !=
						 (size_t)(BLOCKSIZE - (name_len % BLOCKSIZE)))
						{
							return xarc_set_error(x, XARC_MODULE_ERROR,
							 M_UNTAR_TRUNCATED,
							 XC("Unexpected EOF while reading long name"));
						}
					}
				}
				break;
			/* Any other header type is currently not supported, but we should
			 * go ahead and free the last entry's path
			 */
			default:
				if (M_UNTAR(x)->entry_path)
				{
					free(M_UNTAR(x)->entry_path);
					M_UNTAR(x)->entry_path = 0;
				}
				break;
		}
	}
}


/* Section: Module Functions */


/* Function: m_untar_open
 * Open a file as a TAR archive.
 *
 * See also: <XARC_DEFINE_MODULE(name, open_func, extra_size)>
 */
xarc_result_t m_untar_open(xarc* x, const xchar* file, uint8_t type)
{
	/* Clear our own allocated space */
	memset(M_UNTAR(x), 0, sizeof(m_untar_extra));
	/* Set the <xarc> object to use this (mod_untar) implementation of
	 * <handler_funcs>
	 */
	X_BASE(x)->impl = &untar_funcs;

	/* Pass everything off to the decompressor API. If there is a failure, all
	 * necessary state will already be set and we can just return the error
	 * code.
	 */
	xarc_result_t ret = xarc_decompress_open(x, file, type,
	 &M_UNTAR(x)->decomp);
	if (ret != XARC_OK)
		return ret;

	/* Read the headers for the first entry */
	ret = read_tar_headers(x);
	if (ret != XARC_OK)
		return ret;

	return XARC_OK;
}

/* Function: m_untar_close
 * Close an open TAR archive.
 *
 * See also: <handler_funcs.close>
 */
xarc_result_t m_untar_close(xarc* x)
{
	if (M_UNTAR(x)->entry_path)
		free(M_UNTAR(x)->entry_path);
	if (M_UNTAR(x)->decomp)
		M_UNTAR(x)->decomp->close(M_UNTAR(x)->decomp);
	return XARC_OK;
}

/* Function: m_untar_next_item
 * Seek to the next item in a TAR archive.
 *
 * See also: <handler_funcs.next_item>
 */
xarc_result_t m_untar_next_item(xarc* x)
{
	/* If the current item contained file data, the user may not have chosen to
	 * extract it. In that case, skip over it.
	 */
	if (M_UNTAR(x)->entry_bytes_remaining > 0)
	{
		char buf[BLOCKSIZE];
		int32_t br = M_UNTAR(x)->entry_bytes_remaining;
		size_t read_count;
		while (br > 0)
		{
			/* Try to read in a block's worth of data */
			read_count = BLOCKSIZE;
			xarc_result_t ret = M_UNTAR(x)->decomp->read(x, M_UNTAR(x)->decomp,
			 buf, &read_count);
			/* If an error was returned, the <xarc> object's error state was
			 * already set in the decompressor and we can just return the error
			 * code.
			 */
			if (ret != XARC_OK && ret != XARC_DECOMPRESS_EOF)
				return ret;
			/* If we didn't read a full block, the archive is corrupt or
			 * truncated.
			 */
			if (read_count != BLOCKSIZE)
			{
				return xarc_set_error(x, XARC_MODULE_ERROR,
				 M_UNTAR_TRUNCATED, XC("Unexpected EOF while reading tar entry"));
			}
			br -= BLOCKSIZE;
		}
	}

	/* Read the headers for the next entry */
	return read_tar_headers(x);
}

/* Function: m_untar_item_get_info
 * Get the metadata for the current item.
 *
 * See also: <handler_funcs.item_get_info>
 */
xarc_result_t m_untar_item_get_info(xarc* x, xarc_item_info* info)
{
	/* The current entry's metadata has already been read in and stored by
	 * <read_tar_headers>; we just need to copy it over
	 */
	info->path = M_UNTAR(x)->entry_path;
	info->properties = M_UNTAR(x)->entry_properties;
	filesys_time_unix(M_UNTAR(x)->entry_time, &info->mod_time);
	return XARC_OK;
}

/* Function: m_untar_item_extract
 * Extract the current file item to an open FILE stream.
 *
 * See also: <handler_funcs.item_extract>
 */
xarc_result_t m_untar_item_extract(xarc* x, FILE* to, size_t* written)
{
	/* All data will be read and written in TAR blocks */
	char buf[BLOCKSIZE];
	/* Tracks the number of bytes desired, and read, with the decompressor */
	size_t count;

	/* Process data one block at a time until no more data remains to be read */
	while (M_UNTAR(x)->entry_bytes_remaining > 0)
	{
		/* Read in a block from the decompressor */
		count = BLOCKSIZE;
		xarc_result_t ret = M_UNTAR(x)->decomp->read(x, M_UNTAR(x)->decomp, buf,
		 &count);
		/* If we got an error code, the necessary error state has already been
		 * set in the <xarc> object, so just return the code */
		if (ret != XARC_OK && ret != XARC_DECOMPRESS_EOF)
			return ret;
		/* If we didn't get a full block, the archive is corrupt or truncated */
		if (count != BLOCKSIZE)
		{
			return xarc_set_error(x, XARC_MODULE_ERROR,
			 M_UNTAR_TRUNCATED, XC("Unexpected EOF while reading tar entry"));
		}

		/* If we only have a few bytes left for the entry (i.e. less than a full
		 * block's worth), we need to only write the valid bytes */
		if (count > M_UNTAR(x)->entry_bytes_remaining)
			count = M_UNTAR(x)->entry_bytes_remaining;

		/* Subtract up to a block's worth of bytes from the count of how many
		 * are left */
		if (M_UNTAR(x)->entry_bytes_remaining >= BLOCKSIZE)
			M_UNTAR(x)->entry_bytes_remaining -= BLOCKSIZE;
		else
			M_UNTAR(x)->entry_bytes_remaining = 0;

		/* Write the block out to the file; if fwrite fails,
		 * <xarc_set_error_filesys> will set appropriate error state based on
		 * errno.
		 */
		size_t wr = fwrite(buf, 1, count, to);
		if (wr > 0)
			*written += wr;
		if (wr != count)
			return xarc_set_error_filesys(x, 0);
	}

	return XARC_OK;
}

/* Function: m_untar_item_set_props
 * Apply the entry's metadata to the written file.
 *
 * See also: <handler_funcs.item_set_props>
 */
xarc_result_t m_untar_item_set_props(xarc* x, const xchar* path)
{
	/* Apply any metadata that we can convert on the current platform from TAR's
	 * Unix format
	 */
	filesys_set_attributes_unix(path, M_UNTAR(x)->entry_mode);
	filesys_set_modtime_unix(path, M_UNTAR(x)->entry_time);
	return XARC_OK;
}

/* Function: m_untar_error_description
 * Return a string describing the supplied integer error id.
 *
 * See also: <handler_funcs.error_description>
 */
const xchar* m_untar_error_description(xarc* x, int32_t error_id)
{
	/* The error could stem from the decompressor API, in which case we need to
	 * delegate
	 */
	if (X_BASE(x)->error->xarc_id == XARC_DECOMPRESS_ERROR)
		return M_UNTAR(x)->decomp->error_desc(M_UNTAR(x)->decomp, error_id);
	/* Otherwise, describe one of mod_untar's own error codes */
	switch (error_id)
	{
		case M_UNTAR_TRUNCATED:
			return XC("Unexpected EOF while reading TAR file: truncated?");
		case M_UNTAR_CORRUPT:
			return XC("TAR file is invalid or corrupt");
	}
	return XC("");
}
