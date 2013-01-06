/** \file xtest.cpp
 *
 * Created: JohnE, 2013-01-05
 */


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <xarc.hpp>
#include <ctime>
#include <xarc/xarc_exception.hpp>


struct testcb
{
	XARC::ExtractArchive* archive;

	void operator () (const std::wstring& item, uint8_t flags)
	{
		try
		{
			XARC::ExtractItemInfo xi = archive->GetItemInfo();
			time_t t = xi.GetModTime().first;
			wprintf(L"* %s - %s", item.c_str(), _wctime(&t));
		}
		catch (XARC::XarcException& e)
		{
			fwprintf(stderr, L"%s\n", e.GetString().c_str());
		}
		catch (...)
		{
		}
	}
};


#include "mingw-unicode.inc"
int wmain(int argc, wchar_t* argv[])
{
	if (argc < 2)
	{
		fwprintf(stderr, L"Please provide an archive to test\n");
		return -1;
	}

	try
	{
		XARC::ExtractArchive x(argv[1]);
		if (!x.IsOkay())
		{
			fwprintf(stderr, L"(%d) %s\n    %s\n", x.GetLibraryErrorID(),
			 x.GetErrorDescription().c_str(), x.GetErrorAdditional().c_str());
			return -1;
		}

		testcb tcb;
		tcb.archive = &x;
		do
		{
			x.ExtractItem(L"C:\\JDevel\\xarc\\bin\\test_dir",
			 XARC_XFLAG_CALLBACK_DIRS, tcb);
			if (!x.IsOkay())
			{
				fwprintf(stderr, L"%s\n    %s\n",
				 x.GetErrorDescription().c_str(),
				 x.GetErrorAdditional().c_str());
			}
		} while (x.NextItem() == XARC_OK);
	}
	catch (XARC::XarcException& e)
	{
		fwprintf(stderr, L"%s\n", e.GetString().c_str());
		return -1;
	}

	return 0;
}
