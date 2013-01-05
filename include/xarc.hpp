/* Title: XARC C++ user API */

/* File: xarc.hpp
 * The xarc C++ interface.
 */
/* Created: JohnE, 2013-01-04 */

#ifndef XARC_HPP_INC
#define XARC_HPP_INC

#include <string>
#include <utility>
#include <stdexcept>
#include <xarc.h>

namespace XARC
{

#if XARC_NATIVE_WCHAR
typedef std::wstring StringType;
#else
typedef std::string StringType;
#endif

class XarcException : public std::runtime_error
{
public:
	XarcException(const StringType& message)
	 : std::runtime_error("XARC::XarcException"),
	 m_message(message)
	{
	}

	virtual ~XarcException() throw() {}

	const StringType& GetString() const
	{
		return m_message;
	}

private:
	StringType m_message;
};

class Archive
{
public:
	virtual ~Archive() {}

	virtual bool IsOkay() const = 0;
	virtual xarc_result_t GetXarcErrorID() const = 0;
	virtual int32_t GetLibraryErrorID() const = 0;
	virtual StringType GetErrorDescription() const = 0;
	virtual StringType GetErrorAdditional() const = 0;

protected:
	Archive() {}
};

class ExtractItemInfo
{
public:
	StringType GetPath() const;
	bool IsDirectory() const;
	std::pair< uintmax_t, uint32_t > GetModTime() const;

private:
	friend class ExtractArchive;
	ExtractItemInfo(const xarc_item_info* info);

	xarc_item_info m_info;
	StringType m_path;
};

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

class ExtractArchive : public Archive
{
public:
	ExtractArchive();
	ExtractArchive(const xchar* file, uint8_t type = 0);
	virtual ~ExtractArchive();

	virtual bool IsOkay() const;
	virtual xarc_result_t GetXarcErrorID() const;
	virtual int32_t GetLibraryErrorID() const;
	virtual StringType GetErrorDescription() const;
	virtual StringType GetErrorAdditional() const;

	xarc_result_t NextItem();
	ExtractItemInfo GetItemInfo();
	template< class UserCallback >
	ExtractArchive& ExtractItem(const StringType& base_path, uint8_t flags,
	 UserCallback& callback);

private:
	ExtractArchive& ExtractItemUserCallback(const StringType& base_path,
	 uint8_t flags, ExtractCallback* callback);

	xarc* m_xarc;
};

template< class UserCallback >
ExtractArchive& ExtractArchive::ExtractItem(const StringType& base_path,
 uint8_t flags, UserCallback& callback)
{
	ExtractUserCallback< UserCallback > euc(callback);
	return this->ExtractItemUserCallback(base_path, flags, &euc);
}

}

#endif // XARC_HPP_INC
