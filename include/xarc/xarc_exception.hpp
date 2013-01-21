/* File: xarc/xarc_exception.hpp
 * The XARC exception class. (C++ API)
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

#ifndef XARC_EXCEPTION_HPP_INC
#define XARC_EXCEPTION_HPP_INC

#include <stdexcept>
#include <xarc/string_type.hpp>

namespace XARC
{

/* Class: XarcException
 * Exception type thrown by XARC C++ methods.
 *
 * Inherits from std::runtime error in order to be caught by any standard
 * catch clause.
 */
class XarcException : public std::runtime_error
{
public:
	/* Constructor: XarcException
	 * Create a new XarcException with the given error message.
	 */
	XarcException(const StringType& message)
	 : std::runtime_error("XARC::XarcException"),
	 m_message(message)
	{
	}

	/* Destructor: ~XarcException
	 * Virtual destructor.
	 */
	virtual ~XarcException() throw() {}

	/* Method: GetString
	 * Get the exception's specific error message.
	 *
	 * Returns:
	 *   A string containing the specific error message supplied when the
	 *   exception was thrown.
	 */
	const StringType& GetString() const
	{
		return m_message;
	}

private:
	StringType m_message;
};

}

#endif // XARC_EXCEPTION_HPP_INC
