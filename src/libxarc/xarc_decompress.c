/** \file xarc_decompress.c
 *
 * Created: JohnE, 2010-08-02
 */


#include <string.h>
#include <inttypes.h>
#include "xarc_decompress.h"
#include "xarc_impl.h"


typedef struct
{
	const uint8_t* id;
	decomp_open_func* opener;
	const xchar* const* extensions;
} decompressor;


#define XARC_MODULE_BEGIN(module)
#define XARC_HANDLE_1PHASE(type)
#define XARC_HANDLE_2PHASE(type, decomp) \
 extern decomp_open_func XCONCAT2(xarc_decomp_open_func_, decomp);
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

static decompressor decompressors[] = {
#define XARC_MODULE_BEGIN(module)
#define XARC_HANDLE_1PHASE(type)
#define XARC_HANDLE_2PHASE(type, decomp) \
 { &type, &XCONCAT2(xarc_decomp_open_func_, decomp), XCONCAT2(xarc_type_extensions_, type) },
#define XARC_MODULE_END()
#include "modules.inc"
#undef XARC_MODULE_BEGIN
#undef XARC_HANDLE_1PHASE
#undef XARC_HANDLE_2PHASE
#undef XARC_MODULE_END
 { 0, 0, 0 }
};


xarc_result_t xarc_decompress_open(xarc* x, const xchar* path,
 uint8_t type, xarc_decompress_impl** impl)
{
	size_t path_len = 0;
	if (type == 0)
		path_len = xstrlen(path);

	const decompressor* dc;
	for (dc = decompressors; dc->id; ++dc)
	{
		if (type > 0)
		{
			if (type == *dc->id)
				break;
			continue;
		}
		const xchar* const* e;
		for (e = dc->extensions; *e; ++e)
		{
			size_t extn_len = xstrlen(*e);
			if (extn_len > path_len)
				continue;
			if (xstrncasecmp(path + path_len - extn_len, *e, extn_len) == 0)
				break;
		}
		if (*e)
			break;
	}

	if (!dc->id)
	{
		return xarc_set_error(x, XARC_ERR_UNRECOGNIZED_COMPRESSION, 0,
		 XC("File '%s' with type-id %"PRIu8" didn't match any registered decompressors"),
		 path, type);
	}

	return (*dc->opener)(x, path, impl);
}
