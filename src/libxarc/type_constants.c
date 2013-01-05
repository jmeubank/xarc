/** \file type_constants.c
 *
 * Created: JohnE, 2010-07-22
 */


#include <stdint.h>


#define XARC_TYPE_BEGIN(id, name) \
 const uint8_t name = id;
#define XARC_EXTENSION(ext)
#define XARC_TYPE_END()
#include <xarc/types.inc>
#undef XARC_TYPE_BEGIN
#undef XARC_EXTENSION
#undef XARC_TYPE_END
