/* File: libxarc/xarc_impl.c
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
