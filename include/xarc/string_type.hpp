/* File: xarc/string_type.hpp
 * The XARC flexible string type. (C++ API)
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
