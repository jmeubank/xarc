/* File: libxarc/xarc_impl_cxx.cpp
 * The XARC C++ implementation.
 */
/* Created: JohnE, 2013-01-04 */


#include <xarc.hpp>
#include <xarc/xarc_exception.hpp>
#include <cassert>


namespace XARC
{


extern "C" void XarcCxxExtractCallback(void* param, const xchar* path,
 uint8_t properties)
{
	ExtractCallback* cb = static_cast< ExtractCallback* >(param);
	try
	{
		cb->PerformCallback(path, properties);
	}
	catch (...)
	{
		assert(0 && "XARC C++ callbacks must not throw exceptions");
	}
}


StringType ExtractItemInfo::GetPath() const
{
	return m_path;
}

bool ExtractItemInfo::IsDirectory() const
{
	return (m_info.properties & XARC_PROP_DIR);
}

std::pair< uintmax_t, uint32_t > ExtractItemInfo::GetModTime() const
{
	return std::make_pair(m_info.mod_time.seconds, m_info.mod_time.nano);
}

ExtractItemInfo::ExtractItemInfo(const xarc_item_info* info)
{
	m_info = *info;
	m_path = m_info.path;
}


ExtractArchive::ExtractArchive()
 : m_xarc(0)
{
}

ExtractArchive::ExtractArchive(const xchar* file, uint8_t type)
 : m_xarc(0)
{
	this->OpenFile(file, type);
}

ExtractArchive::~ExtractArchive()
{
	if (m_xarc)
		xarc_close(m_xarc);
}


bool ExtractArchive::IsOkay() const
{
	return (m_xarc && xarc_ok(m_xarc));
}

xarc_result_t ExtractArchive::GetXarcErrorID() const
{
	if (!m_xarc)
	{
		throw XarcException(
		 XC("Tried to use ExtractArchive without opening an actual archive")
		);
	}
	return xarc_error_id(m_xarc);
}

int32_t ExtractArchive::GetLibraryErrorID() const
{
	if (!m_xarc)
	{
		throw XarcException(
		 XC("Tried to use ExtractArchive without opening an actual archive")
		);
	}
	return xarc_library_error_id(m_xarc);
}

StringType ExtractArchive::GetErrorDescription() const
{
	if (!m_xarc)
	{
		throw XarcException(
		 XC("Tried to use ExtractArchive without opening an actual archive")
		);
	}
	return StringType(xarc_error_description(m_xarc));
}

StringType ExtractArchive::GetErrorAdditional() const
{
	if (!m_xarc)
	{
		throw XarcException(
		 XC("Tried to use ExtractArchive without opening an actual archive")
		);
	}
	return StringType(xarc_error_additional(m_xarc));
}


xarc_result_t ExtractArchive::OpenFile(const xchar* file, uint8_t type)
{
	if (m_xarc)
		xarc_close(m_xarc);
	m_xarc = xarc_open(file, type);
}

xarc_result_t ExtractArchive::NextItem()
{
	if (!m_xarc)
	{
		throw XarcException(
		 XC("Tried to use ExtractArchive without opening an actual archive")
		);
	}
	return xarc_next_item(m_xarc);
}

ExtractItemInfo ExtractArchive::GetItemInfo()
{
	if (!m_xarc)
	{
		throw XarcException(
		 XC("Tried to use ExtractArchive without opening an actual archive")
		);
	}
	xarc_item_info info;
	if (xarc_item_get_info(m_xarc, &info) != XARC_OK)
		throw XarcException(this->GetErrorDescription());
	return ExtractItemInfo(&info);
}


ExtractArchive& ExtractArchive::ExtractItemUserCallback
 (const StringType& base_path, uint8_t flags, ExtractCallback* callback)
{
	if (!m_xarc)
	{
		throw XarcException(
		 XC("Tried to use ExtractArchive without opening an actual archive")
		);
	}
	xarc_item_extract(m_xarc, base_path.c_str(), flags, XarcCxxExtractCallback,
	 callback);
	return *this;
}


}
