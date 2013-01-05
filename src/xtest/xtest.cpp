/** \file xtest.cpp
 *
 * Created: JohnE, 2013-01-05
 */


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <xarc.hpp>
#include <ctime>


struct testcb
{
	XARC::ExtractArchive* archive;

	void operator () (const std::string& item, uint8_t flags)
	{
		try
		{
			XARC::ExtractItemInfo xi = archive->GetItemInfo();
			time_t t = xi.GetModTime().first;
			printf("* %s - %s", item.c_str(), ctime(&t));
		}
		catch (XARC::XarcException& e)
		{
			fprintf(stderr, "%s\n", e.GetString().c_str());
		}
		catch (...)
		{
		}
	}
};


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Please provide an archive to test\n");
		return -1;
	}

	try
	{
		XARC::ExtractArchive x(argv[1]);
		if (!x.IsOkay())
		{
			fprintf(stderr, "(%d) %s\n    %s\n", x.GetLibraryErrorID(),
			 x.GetErrorDescription().c_str(), x.GetErrorAdditional().c_str());
			return -1;
		}

		testcb tcb;
		tcb.archive = &x;
		do
		{
			x.ExtractItem("C:\\JDevel\\xarc\\bin\\test_dir",
			 XARC_XFLAG_CALLBACK_DIRS, tcb);
			if (!x.IsOkay())
			{
				fprintf(stderr, "%s\n    %s\n", x.GetErrorDescription().c_str(),
				 x.GetErrorAdditional().c_str());
			}
		} while (x.NextItem() != XARC_NO_MORE_ITEMS);
	}
	catch (XARC::XarcException& e)
	{
		fprintf(stderr, "%s\n", e.GetString().c_str());
		return -1;
	}

	return 0;
}
