/** \file test_linux.c
 *
 * Created: JohnE, 2010-09-09
 */


#include <stdio.h>
#include <malloc.h>
#include <xarc.h>
#include <time.h>


void testcb(void* param __attribute__((unused)), const char* item, uint8_t flags __attribute__((unused)))
{
	xarc_item_info xi;
	if (xarc_item_get_info(param, &xi) == XARC_OK)
	{
		time_t t = xi.mod_time.seconds;
		printf("* %s  %s", item, ctime(&t));
	}
}


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Please provide an archive to test\n");
		return -1;
	}

	xarc* x = xarc_open(argv[1], 0);
	if (!xarc_ok(x))
	{
		fprintf(stderr, "%s\n    %s\n", xarc_error_description(x),
		 xarc_error_additional(x));
		xarc_close(x);
		return -1;
	}

	do
	{
		xarc_result_t xr = xarc_item_extract(x, "./test_dir",
		 XARC_XFLAG_CALLBACK_DIRS, testcb, x);
		if (xr != XARC_OK)
		{
			fprintf(stderr, "%s\n    %s\n", xarc_error_description(x),
			 xarc_error_additional(x));
		}
	} while (xarc_next_item(x) == XARC_OK);

	xarc_close(x);

	return 0;
}
