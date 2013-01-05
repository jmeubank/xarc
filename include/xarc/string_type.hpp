/* File: xarc/string_type.hpp
 * The XARC flexible string type. (C++ API)
 */
/* Created: JohnE, 2013-01-05 */

#ifndef XARC_STRING_TYPE_HPP_INC
#define XARC_STRING_TYPE_HPP_INC

namespace XARC
{

/* Type: StringType
 * std::string or std::wstring.
 *
 * Typedef std::string or std::wstring as StringType, based on whether XARC is
 *   compiled to use strings with 8-bit characters (char) or 16-bit characters
 *   (wchar_t).
 */
#if XARC_NATIVE_WCHAR
typedef std::wstring StringType;
#else
typedef std::string StringType;
#endif

}

#endif //XARC_STRING_TYPE_HPP_INC
