/* File: xarc/extract_helpers.hpp
 * Helper classes for ExtractArchive. (C++ API)
 */
/* Created: JohnE, 2013-01-05 */

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
