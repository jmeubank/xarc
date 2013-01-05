/** \file mod_untar.c
 *
 * Created: JohnE, 2010-08-02
 * Based on contrib/untgz/untgz.c from the zlib distribution
 */


#include <string.h>
#include <malloc.h>
#include "xarc_impl.h"
#include "xarc_decompress.h"
#include "filesys.h"


typedef struct
{
	xarc_decompress_impl* decomp;
	uint8_t entry_properties;
	size_t entry_bytes_remaining;
	xchar* entry_path;
	int32_t entry_time;
	int32_t entry_mode;
} m_untar_extra;
#define M_UNTAR(x) ((m_untar_extra*)((void*)x + sizeof(struct _xarc)))


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


XARC_DEFINE_MODULE(mod_untar, m_untar_open, sizeof(m_untar_extra))


#define M_UNTAR_TRUNCATED	-1
#define M_UNTAR_CORRUPT		-2

handler_funcs untar_funcs = {
	m_untar_close,
	m_untar_next_item,
	m_untar_item_get_info,
	m_untar_item_extract,
	m_untar_item_set_props,
	m_untar_error_description
};


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

static xarc_result_t read_tar_headers(xarc* x)
{
	if (M_UNTAR(x)->entry_path)
	{
		free(M_UNTAR(x)->entry_path);
		M_UNTAR(x)->entry_path = 0;
	}
	M_UNTAR(x)->entry_properties = 0;

	size_t read_count;
	struct tar_header th;
	while (1)
	{
		read_count = BLOCKSIZE;
		xarc_result_t ret = M_UNTAR(x)->decomp->read(x, M_UNTAR(x)->decomp, &th,
		 &read_count);
		if (ret != XARC_OK && ret != XARC_DECOMPRESS_EOF)
			return ret;
		if (read_count == 0)
		{
			return xarc_set_error(x, XARC_NO_MORE_ITEMS, 0,
			 XC("EOF reached on TAR archive"));
		}
		if (read_count != BLOCKSIZE)
		{
			return xarc_set_error(x, XARC_MODULE_ERROR, M_UNTAR_TRUNCATED,
			 XC("Unable to read full tar header block"));
		}

		if (th.name[0] == 0)
			return XARC_NO_MORE_ITEMS;

		M_UNTAR(x)->entry_mode = untgz_getoct(th.mode, 8);
		M_UNTAR(x)->entry_time = untgz_getoct(th.mtime, 12);
		if (M_UNTAR(x)->entry_mode == -1 || M_UNTAR(x)->entry_time == -1)
		{
			return xarc_set_error(x, XARC_MODULE_ERROR, M_UNTAR_CORRUPT,
			 XC("Invalid values in tar header"));
		}

		if (th.typeflag != GNUTYPE_LONGLINK && th.typeflag != GNUTYPE_LONGNAME
		 && !M_UNTAR(x)->entry_path)
		{
			char path8[SHORTNAMESIZE + 1];
			strncpy(path8, th.name, SHORTNAMESIZE);
			path8[SHORTNAMESIZE] = '\0';
			size_t lenl = filesys_localize_utf8(path8, -1, 0, 0);
			M_UNTAR(x)->entry_path = malloc(sizeof(xchar) * lenl);
			filesys_localize_utf8(path8, -1, M_UNTAR(x)->entry_path, lenl);
		}

		switch (th.typeflag)
		{
			case DIRTYPE:
				M_UNTAR(x)->entry_properties |= XARC_PROP_DIR;
				M_UNTAR(x)->entry_bytes_remaining = 0;
				return XARC_OK;
			case REGTYPE:
			case AREGTYPE:
				{
					int32_t fsize = untgz_getoct(th.size, 12);
					if (fsize < 0)
					{
						return xarc_set_error(x, XARC_MODULE_ERROR,
						 M_UNTAR_CORRUPT, XC("Invalid value for size of entry"));
					}
					M_UNTAR(x)->entry_bytes_remaining = fsize;
					return XARC_OK;
				}
			case GNUTYPE_LONGLINK:
			case GNUTYPE_LONGNAME:
				{
					int32_t name_len = untgz_getoct(th.size, 12);
					if (name_len < 1)
					{
						return xarc_set_error(x, XARC_MODULE_ERROR,
						 M_UNTAR_CORRUPT, XC("Invalid value for length of long name"));
					}

					char* path8 = malloc(name_len + 1);

					read_count = name_len;
					ret = M_UNTAR(x)->decomp->read(x, M_UNTAR(x)->decomp, path8,
					 &read_count);
					if (ret != XARC_OK && ret != XARC_DECOMPRESS_EOF)
						return ret;
					if (read_count != (size_t)name_len)
					{
						return xarc_set_error(x, XARC_MODULE_ERROR,
						 M_UNTAR_TRUNCATED,
						 XC("Unexpected EOF while reading long name"));
					}
					path8[name_len] = '\0';

					size_t lenl = filesys_localize_utf8(path8, name_len + 1, 0,
					 0);
					M_UNTAR(x)->entry_path = realloc(M_UNTAR(x)->entry_path,
					 sizeof(xchar) * lenl);
					filesys_localize_utf8(path8, name_len + 1,
					 M_UNTAR(x)->entry_path, lenl);
					free(path8);

					read_count = BLOCKSIZE - (name_len % BLOCKSIZE);
					ret = M_UNTAR(x)->decomp->read(x, M_UNTAR(x)->decomp, &th,
					 &read_count);
					if (ret != XARC_OK && ret != XARC_DECOMPRESS_EOF)
						return ret;
					if (read_count != (size_t)(BLOCKSIZE - (name_len % BLOCKSIZE)))
					{
						return xarc_set_error(x, XARC_MODULE_ERROR,
						 M_UNTAR_TRUNCATED,
						 XC("Unexpected EOF while reading long name"));
					}
				}
				break;
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


xarc_result_t m_untar_open(xarc* x, const xchar* file, uint8_t type)
{
	memset(M_UNTAR(x), 0, sizeof(m_untar_extra));
	X_BASE(x)->impl = &untar_funcs;

	xarc_result_t ret = xarc_decompress_open(x, file, type,
	 &M_UNTAR(x)->decomp);
	if (ret != XARC_OK)
		return ret;

	ret = read_tar_headers(x);
	if (ret != XARC_OK)
		return ret;

	return XARC_OK;
}

xarc_result_t m_untar_close(xarc* x)
{
	if (M_UNTAR(x)->entry_path)
		free(M_UNTAR(x)->entry_path);
	if (M_UNTAR(x)->decomp)
		M_UNTAR(x)->decomp->close(M_UNTAR(x)->decomp);
	return XARC_OK;
}

xarc_result_t m_untar_next_item(xarc* x)
{
	if (M_UNTAR(x)->entry_bytes_remaining > 0)
	{
		char buf[BLOCKSIZE];
		int32_t br = M_UNTAR(x)->entry_bytes_remaining;
		size_t read_count;
		while (br > 0)
		{
			read_count = BLOCKSIZE;
			xarc_result_t ret = M_UNTAR(x)->decomp->read(x, M_UNTAR(x)->decomp,
			 buf, &read_count);
			if (ret != XARC_OK && ret != XARC_DECOMPRESS_EOF)
				return ret;
			if (read_count != BLOCKSIZE)
			{
				return xarc_set_error(x, XARC_MODULE_ERROR,
				 M_UNTAR_TRUNCATED, XC("Unexpected EOF while reading tar entry"));
			}
			br -= BLOCKSIZE;
		}
	}

	return read_tar_headers(x);
}

xarc_result_t m_untar_item_get_info(xarc* x, xarc_item_info* info)
{
	info->path.native = M_UNTAR(x)->entry_path;
	info->properties = M_UNTAR(x)->entry_properties;
	filesys_time_unix(M_UNTAR(x)->entry_time, &info->mod_time);
	return XARC_OK;
}

xarc_result_t m_untar_item_extract(xarc* x, FILE* to, size_t* written)
{
	char buf[BLOCKSIZE];
	size_t count;

	while (M_UNTAR(x)->entry_bytes_remaining > 0)
	{
		count = BLOCKSIZE;
		xarc_result_t ret = M_UNTAR(x)->decomp->read(x, M_UNTAR(x)->decomp,
		 buf, &count);
		if (ret != XARC_OK && ret != XARC_DECOMPRESS_EOF)
			return ret;
		if (count != BLOCKSIZE)
		{
			return xarc_set_error(x, XARC_MODULE_ERROR,
			 M_UNTAR_TRUNCATED, XC("Unexpected EOF while reading tar entry"));
		}

		if (count > M_UNTAR(x)->entry_bytes_remaining)
			count = M_UNTAR(x)->entry_bytes_remaining;

		if (M_UNTAR(x)->entry_bytes_remaining >= BLOCKSIZE)
			M_UNTAR(x)->entry_bytes_remaining -= BLOCKSIZE;
		else
			M_UNTAR(x)->entry_bytes_remaining = 0;

		size_t wr = fwrite(buf, 1, count, to);
		if (wr > 0)
			*written += wr;
		if (wr != count)
			return xarc_set_error_filesys(x, 0);
	}

	return XARC_OK;
}

xarc_result_t m_untar_item_set_props(xarc* x, const xchar* path)
{
	filesys_set_attributes_unix(path, M_UNTAR(x)->entry_mode);
	filesys_set_modtime_unix(path, M_UNTAR(x)->entry_time);
	return XARC_OK;
}

const xchar* m_untar_error_description(xarc* x, int32_t error_id)
{
	if (X_BASE(x)->error->xarc_id == XARC_DECOMPRESS_ERROR)
		return M_UNTAR(x)->decomp->error_desc(M_UNTAR(x)->decomp, error_id);
	switch (error_id)
	{
		case M_UNTAR_TRUNCATED:
			return XC("Unexpected EOF while reading TAR file: truncated?");
		case M_UNTAR_CORRUPT:
			return XC("TAR file is invalid or corrupt");
	}
	return XC("");
}
