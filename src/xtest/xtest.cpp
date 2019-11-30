/* File: xtest/xtest.cpp
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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <xarc.hpp>
#include <ctime>
#include <xarc/xarc_exception.hpp>


#if _UNICODE
#	define xprintf wprintf
#	define xctime _wctime
#	define fxprintf fwprintf
#	define xmain wmain
#else
#	define xprintf printf
#	define xctime ctime
#	define fxprintf fprintf
#	define xmain main
#endif


struct testcb
{
	XARC::ExtractArchive* archive;

	void operator () (const XARC::StringType& item, uint8_t flags)
	{
		try
		{
			XARC::ExtractItemInfo xi = archive->GetItemInfo();
			time_t t = xi.GetModTime().first;
			xprintf(XC("* %s - %s"), item.c_str(), xctime(&t));
		}
		catch (XARC::XarcException& e)
		{
			fxprintf(stderr, XC("%s\n"), e.GetString().c_str());
		}
		catch (...)
		{
		}
	}
};


#if _UNICODE

#	ifndef __MSVCRT__
#		error Must link with MSVCRT for Unicode wmain
#	endif

#	include <wchar.h>
#	include <stdlib.h>

	extern int _CRT_glob;

#	ifdef __cplusplus
		extern "C" void __wgetmainargs(int*, wchar_t***, wchar_t***, int, int*);
#	else
		void __wgetmainargs(int*, wchar_t***, wchar_t***, int, int*);
#	endif

#endif /* defined(_UNICODE) */

int xmain(int argc, xchar* argv[])
{
	if (argc < 2)
	{
		fxprintf(stderr, XC("Please provide an archive to test\n"));
		return -1;
	}

	try
	{
		XARC::ExtractArchive x(argv[1]);
		if (!x.IsOkay())
		{
			fxprintf(stderr, XC("(%d) %s\n    %s\n"), x.GetLibraryErrorID(),
			 x.GetErrorDescription().c_str(), x.GetErrorAdditional().c_str());
			return -1;
		}

		testcb tcb;
		tcb.archive = &x;
		do
		{
			x.ExtractItem(XC("C:\\Users\\joeub\\xarc\\build-cmd\\test_dir"),
			 XARC_XFLAG_CALLBACK_DIRS, tcb);
			if (!x.IsOkay())
			{
				fxprintf(stderr, XC("%s\n    %s\n"),
				 x.GetErrorDescription().c_str(),
				 x.GetErrorAdditional().c_str());
			}
		} while (x.NextItem() == XARC_OK);
	}
	catch (XARC::XarcException& e)
	{
		fxprintf(stderr, XC("%s\n"), e.GetString().c_str());
		return -1;
	}

	return 0;
}

#if _UNICODE
	int main()
	{
		wchar_t** envp;
		wchar_t** argv;
		int argc;
		int si = 0;
		__wgetmainargs(&argc, &argv, &envp, _CRT_glob, &si);
		return wmain(argc, argv);
	}
#endif
