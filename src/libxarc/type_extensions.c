/** \file type_extensions.c
 *
 * Created: JohnE, 2010-08-05
 */


#include <xarc/xchar.h>


#ifndef XCONCAT2
#define INNER_CONCAT2(a, b) a ## b
#define XCONCAT2(a, b) INNER_CONCAT2(a, b)
#endif


#define XARC_TYPE_BEGIN(id, name) \
 const xchar* const XCONCAT2(xarc_type_extensions_, name)[] = {
#define XARC_EXTENSION(ext) \
 XC(ext),
#define XARC_TYPE_END() \
 0 };
#include <xarc/types.inc>
#undef XARC_TYPE_BEGIN
#undef XARC_EXTENSION
#undef XARC_TYPE_END
