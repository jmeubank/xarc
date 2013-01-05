/** \file xarc_base.c
 *
 * Created: JohnE, 2010-07-20
 */


#include <string.h>
#include <inttypes.h>
#include <malloc.h>
#include <sys/stat.h>
#include "xarc_impl.h"
#include "filesys.h"

#if defined(_WIN32) && __MSVCRT_VERSION__ < 0x0700
#include <windows.h>
#include <direct.h>
#endif


typedef xarc_result_t (*open_func)(xarc*, const xchar*, uint8_t);
typedef struct
{
	const uint8_t* id;
	const xchar* const* extensions;
} arctype;
typedef struct
{
	open_func* opener;
	const size_t* extra_size;
	const arctype* mod_types;
} module;


#define XARC_MODULE_BEGIN(module) \
 extern const size_t XCONCAT2(xarc_extra_size_, module); \
 extern open_func XCONCAT2(xarc_open_func_, module);
#define XARC_HANDLE_1PHASE(type)
#define XARC_HANDLE_2PHASE(type, decomp)
#define XARC_MODULE_END()
#include "modules.inc"
#undef XARC_MODULE_BEGIN
#undef XARC_HANDLE_1PHASE
#undef XARC_HANDLE_2PHASE
#undef XARC_MODULE_END

#define XARC_TYPE_BEGIN(id, name) \
 extern const xchar* const XCONCAT2(xarc_type_extensions_, name)[];
#define XARC_EXTENSION(ext)
#define XARC_TYPE_END()
#include <xarc/types.inc>
#undef XARC_TYPE_BEGIN
#undef XARC_EXTENSION
#undef XARC_TYPE_END


#define XARC_MODULE_BEGIN(module) \
 static arctype XCONCAT2(mod_types_, module)[] = {
#define XARC_HANDLE_1PHASE(type) \
 { &type, XCONCAT2(xarc_type_extensions_, type) },
#define XARC_HANDLE_2PHASE(type, decomp) \
 { &type, XCONCAT2(xarc_type_extensions_, type) },
#define XARC_MODULE_END() \
 { 0, 0 } };
#include "modules.inc"
#undef XARC_MODULE_BEGIN
#undef XARC_HANDLE_1PHASE
#undef XARC_HANDLE_2PHASE
#undef XARC_MODULE_END

static module modules[] = {
#define XARC_MODULE_BEGIN(module) \
 { &XCONCAT2(xarc_open_func_, module), &XCONCAT2(xarc_extra_size_, module), XCONCAT2(mod_types_, module) },
#define XARC_HANDLE_1PHASE(type)
#define XARC_HANDLE_2PHASE(type, decomp)
#define XARC_MODULE_END()
#include "modules.inc"
#undef XARC_MODULE_BEGIN
#undef XARC_HANDLE_1PHASE
#undef XARC_HANDLE_2PHASE
#undef XARC_MODULE_END
 { 0, 0, 0 }
};


static void x_init_base(struct _xarc* x)
{
	memset(x, 0, sizeof(struct _xarc));
}

static xarc_result_t recurse_ensure_dir(xarc* x, xchar* full_path,
 size_t base_len, size_t this_stop, uint8_t flags,
 xarc_extract_callback callback, void* callback_param)
{
	xchar save = full_path[this_stop];
	full_path[this_stop] = XC('\0');

	if (!filesys_dir_exists(full_path))
	{
		if (this_stop <= base_len)
		{
			return xarc_set_error(x, XARC_ERR_NO_BASE_PATH, 0,
			 XC("Trying to create '%s'"), full_path);
		}

		size_t prev_stop = this_stop - 1;
		while (!filesys_is_dir_sep(full_path[prev_stop]) && prev_stop > base_len)
			--prev_stop;
		xarc_result_t ret = recurse_ensure_dir(x, full_path, base_len,
		 prev_stop, flags, callback, callback_param);
		if (ret != XARC_OK)
			return ret;
		if (filesys_mkdir(full_path) != 0)
		{
			return xarc_set_error_filesys(x, XC("Trying to create '%s'"),
			 full_path);
		}
		if (callback && (flags & XARC_XFLAG_CALLBACK_DIRS))
			callback(callback_param, full_path + base_len, XARC_PROP_DIR);
	}

	full_path[this_stop] = save;
	return XARC_OK;
}


xarc* xarc_open(const xchar* file, uint8_t type)
{
	size_t file_len = 0;
	if (type == 0)
		file_len = xstrlen(file);

	const module* m;
	for (m = modules; m->opener; ++m)
	{
		const arctype* at;
		for (at = m->mod_types; at->id; ++at)
		{
			if (type > 0)
			{
				if (type == *at->id)
					break;
				continue;
			}
			const xchar* const* e;
			for (e = at->extensions; *e; ++e)
			{
				size_t extn_len = xstrlen(*e);
				if (extn_len > file_len)
					continue;
				if (xstrncasecmp(file + file_len - extn_len, *e, extn_len) == 0)
					break;
			}
			if (*e)
				break;
		}
		if (at->id)
			break;
	}

	if (!m->opener)
	{
		struct _xarc* x = malloc(sizeof(struct _xarc));
		x_init_base(x);
		xarc_set_error(x, XARC_ERR_UNRECOGNIZED_ARCHIVE, 0,
		 XC("File '%s' with type-id %"PRIu8" didn't match any registered handlers"),
		 file, type);
		return x;
	}

	struct _xarc* x = malloc(sizeof(struct _xarc) + *m->extra_size);
	x_init_base(x);
	(*m->opener)(x, file, type);
	return x;
}

xarc_result_t xarc_close(xarc* x)
{
	if (!x)
		return XARC_OK;
	xarc_result_t ret = XARC_OK;
	if (X_BASE(x)->impl)
		ret = X_BASE(x)->impl->close(x);
	if (X_BASE(x)->error)
	{
		if (X_BASE(x)->error->error_additional)
			free(X_BASE(x)->error->error_additional);
		free(X_BASE(x)->error);
	}
	free(x);
	return ret;
}

int8_t xarc_ok(xarc* x)
{
	return (X_BASE(x)->error) ? 0 : 1;
}

int32_t xarc_library_error_id(xarc* x)
{
	return X_BASE(x)->error ? X_BASE(x)->error->library_error_id : 0;
}

const xchar* xarc_error_description(xarc* x)
{
	if (!X_BASE(x)->error)
		return XC("");
	switch (X_BASE(x)->error->xarc_id)
	{
		case XARC_NO_MORE_ITEMS:
			return XC("No more items in the archive");
		case XARC_DECOMPRESS_EOF:
			return XC("EOF reached in decompression stream");
		case XARC_MODULE_ERROR:
		case XARC_DECOMPRESS_ERROR:
			return X_BASE(x)->impl->error_description(x, X_BASE(x)->error->library_error_id);
		case XARC_FILESYSTEM_ERROR:
#if XARC_NATIVE_WCHAR && defined(_WIN32) && __MSVCRT_VERSION__ < 0x0700
			{
				const char* str8 = strerror(X_BASE(x)->error->library_error_id);
				size_t len16 = filesys_localize_char(str8, -1, 0, 0);
				X_BASE(x)->error->error_additional
				 = malloc(sizeof(wchar_t) * len16);
				filesys_localize_char(str8, -1,
				 X_BASE(x)->error->error_additional, len16);
			}
			return X_BASE(x)->error->error_additional;
#else
			return xstrerror(X_BASE(x)->error->library_error_id);
#endif
		case XARC_ERR_UNRECOGNIZED_ARCHIVE:
			return XC("None of the xarc handlers recognized this archive");
		case XARC_ERR_NOT_VALID_ARCHIVE:
			return XC("The file is not a valid archive for this handler");
		case XARC_ERR_DIR_IS_FILE:
			return XC("The path already exists as a file");
		case XARC_ERR_NO_BASE_PATH:
			return XC("The base path doesn't exist");
		case XARC_OK:
		default:
			return XC("");
	}
}

const xchar* xarc_error_additional(xarc* x)
{
	if (!X_BASE(x)->error || !X_BASE(x)->error->error_additional)
		return XC("");
	return X_BASE(x)->error->error_additional;
}

xarc_result_t xarc_next_item(xarc* x)
{
	if (X_BASE(x)->error)
		return X_BASE(x)->error->xarc_id;
	return X_BASE(x)->impl->next_item(x);
}

xarc_result_t xarc_item_get_info(xarc* x, xarc_item_info* info)
{
	if (X_BASE(x)->error)
		return X_BASE(x)->error->xarc_id;
	return X_BASE(x)->impl->item_get_info(x, info);
}

xarc_result_t xarc_item_extract(xarc* x, const xchar* base_path, uint8_t flags,
 xarc_extract_callback callback, void* callback_param)
{
	if (X_BASE(x)->error)
		return X_BASE(x)->error->xarc_id;

	if (!filesys_dir_exists(base_path))
	{
		return xarc_set_error(x, XARC_ERR_NO_BASE_PATH, 0,
		 XC("Cannot extract to nonexistent base path '%s'"), base_path);
	}

	xarc_item_info xi;
	xarc_result_t ret = X_BASE(x)->impl->item_get_info(x, &xi);
	if (ret != XARC_OK)
		return ret;

	size_t base_len = xstrlen(base_path);
	size_t item_len = xstrlen(xi.path.native);
	// Allocate buffer to hold full item path
	xchar* full_path = malloc(sizeof(xchar) * (base_len + item_len + 2));
	// Copy base path to buffer
	xstrcpy(full_path, base_path);
	// Ensure a trailing path separator
	if (!filesys_is_dir_sep(full_path[base_len - 1]))
	{
		full_path[base_len] = '/';
		++base_len;
	}
	// Copy item path after base path in buffer
	xstrcpy(full_path + base_len, xi.path.native);

	// Get directory portion of item path: dir_stop will be the index of the
	// last char in the path's directory portion
	size_t dir_stop = base_len + item_len - 1;
	// If this item is a real file, drop the filename
	if (!(xi.properties & XARC_PROP_DIR))
	{
		while (!filesys_is_dir_sep(full_path[dir_stop]) && dir_stop > base_len)
			--dir_stop;
	}
	// Drop any trailing path separators
	while (filesys_is_dir_sep(full_path[dir_stop]) && dir_stop > base_len)
		--dir_stop;

	if (dir_stop > base_len) //we have one or more subdirectories; create them
	{
		ret = recurse_ensure_dir(x, full_path, base_len,
		 dir_stop + 1, flags, callback, callback_param);
		if (ret != XARC_OK)
		{
			free(full_path);
			return ret;
		}
	}

	if (!(xi.properties & XARC_PROP_DIR))
	{
		/* Open the file for output */
		filesys_ensure_writable(full_path);
		FILE* outfile = xfopen(full_path, XC("wb"));
		if (!outfile)
		{
			ret = xarc_set_error_filesys(x,
			 XC("Couldn't open file '%s'"), full_path);
			free(full_path);
			return ret;
		}

		/* Run the module's decompressor */
		size_t written;
		ret = X_BASE(x)->impl->item_extract(x, outfile, &written);
		fclose(outfile);
		if (ret != XARC_OK)
		{
			free(full_path);
			return ret;
		}
	}

	/* Set the file/directory properties */
	ret = X_BASE(x)->impl->item_set_props(x, full_path);
	if (ret != XARC_OK)
	{
		free(full_path);
		return ret;
	}

	/* Run the callback if requested (unless this entry is a directory, and the
	 * callback has already been run in recurse_ensure_dir)
	 */
	if (callback && !(xi.properties & XARC_PROP_DIR))
		callback(callback_param, full_path + base_len, 0);

	free(full_path);

	return XARC_OK;
}

