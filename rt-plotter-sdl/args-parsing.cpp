#include "args-parsing.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>

cmd_options_t gl_options = {
	0,	//spread lines
	230400,	//baud rate
	0.5,	//yscale,
	0,	//print values to console
	0, //print values to console ONLY (no actual plotting!)
	0, //print in parser flag active
	0,	//xy mode
	0,	//csv header
	0,	//offaxis-encoder
	1	//fsr-sensor
};

std::string gl_csvheader;
static char printstr[128] = { 0 };
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
			if (strcmp("--csv-header", argv[i]) == 0)
			{
				popts->csv_header = 1;
				gl_csvheader = argv[i + 1];
				printf("%s\n", gl_csvheader.c_str());
			}
		}
		for (int i = 1; i < argc; i++)
		{
			if (strcmp("--spread-lines", argv[i]) == 0)
			{
				popts->spread_lines = 1;
				sprintf_s(printstr, sizeof(printstr), "Spreading lines\r\n");
			}
			if (strcmp("--baudrate", argv[i]) == 0)
			{
				
				if (argc > (i + 1))
				{
					int value = strtol(argv[i + 1], &tmp, 10);
					popts->baud_rate = value;
					sprintf_s(printstr, sizeof(printstr),  "Overriding baudrate as %d\r\n", popts->baud_rate);
				}
				else
				{
					sprintf_s(printstr, sizeof(printstr),  "invalid baudrate format\r\n");
				}
			}
			if (strcmp("--yscale", argv[i]) == 0)
			{
				if (argc > (i + 1))
				{
					float value = strtof(argv[i + 1], &tmp);
					popts->yscale = value;
					sprintf_s(printstr, sizeof(printstr),  "Overriding yscale as %f\r\n", popts->yscale);
				}
				else
				{
					sprintf_s(printstr, sizeof(printstr),  "invalid yscale format\r\n");
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
				sprintf_s(printstr, sizeof(printstr),  "Using xy mode...\r\n");
			}
			if (strcmp("--offaxis-encoder", argv[i]) == 0)
			{
				popts->offaxis_encoder = 1;
				sprintf_s(printstr, sizeof(printstr), "Parsing as offaxis encoder\r\n");
			}
			if (popts->csv_header == 0)
			{
				printf(printstr);
				memset(printstr, 0, sizeof(printstr));
			}
		}
	}
}
