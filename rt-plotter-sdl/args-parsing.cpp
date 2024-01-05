#include "args-parsing.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

cmd_options_t gl_options = {
	0,	//spread lines
	921600,	//baud rate
	400.f/4096.f,	//yscale,
	0,	//print values to console
	0, //print values to console ONLY (no actual plotting!)
	0, //print in parser flag active
	0	//xy mode
};

/*
* Parse any and all arguments coming in
*/
void parse_args(int argc, char* argv[], cmd_options_t * popts)
{
	if (argc > 1)
	{
		char* tmp;
		for (int i = 1; i < argc; i++)
		{
			if (strcmp("--spread-lines", argv[i]) == 0)
			{
				popts->spread_lines = 1;
				printf("Spreading lines\r\n");
			}
			if (strcmp("--baudrate", argv[i]) == 0)
			{
				
				if (argc > (i + 1))
				{
					int value = strtol(argv[i + 1], &tmp, 10);
					popts->baud_rate = value;
					printf("Overriding baudrate as %d\r\n", popts->baud_rate);
				}
				else
				{
					printf("invalid baudrate format\r\n");
				}
			}
			if (strcmp("--yscale", argv[i]) == 0)
			{
				if (argc > (i + 1))
				{
					float value = strtof(argv[i + 1], &tmp);
					popts->yscale = value;
					printf("Overriding yscale as %f\r\n", popts->yscale);
				}
				else
				{
					printf("invalid yscale format\r\n");
				}
			}
			if (strcmp("--printvals", argv[i]) == 0)
			{
				popts->print_vals = 1;
			}
			if (strcmp("--print-only", argv[i]) == 0)
			{
				popts->print_only = 1;
			}
			if (strcmp("--print-in-parser", argv[i]) == 0)
			{
				popts->print_in_parser = 1;
			}
			if (strcmp("--xy-mode", argv[i]) == 0)
			{
				popts->xy_mode = 1;
				printf("Using xy mode...\r\n");
			}
		}
	}
}
