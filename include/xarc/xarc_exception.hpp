/* File: xarc/xarc_exception.hpp
 * The XARC exception class. (C++ API)
 */
/* Created: JohnE, 2013-01-05 */

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
