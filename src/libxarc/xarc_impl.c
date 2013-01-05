/** \file xarc_impl.c
 *
 * Created: JohnE, 2010-07-20
 */


#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include "xarc_impl.h"


xarc_result_t xarc_set_error(xarc* x, xarc_result_t status,
 int32_t library_error_id, const xchar* addl_fmt, ...)
{
	if (!X_BASE(x)->error)
	{
		X_BASE(x)->error = malloc(sizeof(xarc_error));
		X_BASE(x)->error->error_additional = 0;
	}
	X_BASE(x)->error->xarc_id = status;
	X_BASE(x)->error->library_error_id = library_error_id;
	if (addl_fmt)
	{
		if (!X_BASE(x)->error->error_additional)
			X_BASE(x)->error->error_additional = malloc(sizeof(xchar) * 2048);
		va_list vl;
		va_start(vl, addl_fmt);
		xvsnprintf(X_BASE(x)->error->error_additional, 2048, addl_fmt, vl);
		va_end(vl);
	}
	else if (X_BASE(x)->error->error_additional)
	{
		free(X_BASE(x)->error->error_additional);
		X_BASE(x)->error->error_additional = 0;
	}
	return status;
}

xarc_result_t xarc_set_error_filesys(xarc* x, const xchar* addl_fmt, ...)
{
	if (!X_BASE(x)->error)
	{
		X_BASE(x)->error = malloc(sizeof(xarc_error));
		X_BASE(x)->error->error_additional = 0;
	}
	X_BASE(x)->error->xarc_id = XARC_FILESYSTEM_ERROR;
	X_BASE(x)->error->library_error_id = errno;
	if (addl_fmt)
	{
		if (!X_BASE(x)->error->error_additional)
			X_BASE(x)->error->error_additional = malloc(sizeof(xchar) * 2048);
		va_list vl;
		va_start(vl, addl_fmt);
		xvsnprintf(X_BASE(x)->error->error_additional, 2048, addl_fmt, vl);
		va_end(vl);
	}
	else if (X_BASE(x)->error->error_additional)
	{
		free(X_BASE(x)->error->error_additional);
		X_BASE(x)->error->error_additional = 0;
	}
	return XARC_FILESYSTEM_ERROR;
}
