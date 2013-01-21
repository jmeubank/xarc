/* File: xarc/extract_helpers.hpp
 * Helper classes for ExtractArchive. (C++ API)
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

#ifndef XARC_EXTRACT_HELPERS_HPP_INC
#define XARC_EXTRACT_HELPERS_HPP_INC

namespace XARC
{

class ExtractCallback
{
public:
	virtual void PerformCallback(const StringType& path, uint8_t properties)
	 = 0;

protected:
	ExtractCallback() {}
};

template< class UserCallback >
class ExtractUserCallback : public ExtractCallback
{
public:
	virtual void PerformCallback(const StringType& path, uint8_t properties)
	{
		m_cb(path, properties);
	}

private:
	friend class ExtractArchive;

	ExtractUserCallback() {}
	ExtractUserCallback(UserCallback& cb)
	 : m_cb(cb)
	{}

	UserCallback& m_cb;
};

}

#endif // XARC_EXTRACT_HELPERS_HPP_INC
