#include "args-parsing.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>

cmd_options_t gl_options = {
	0,	//spread lines
	921600,	//baud rate
	400/(4096.f),	//yscale,
	0,	//print values to console
	0, //print values to console ONLY (no actual plotting!)
	0, //print in parser flag active
	0,	//xy mode
	0,	//csv header
	(1.f/4096.f), //console scaler
	1,	//integer modulo for plotting printing
	3,	//number of widths
	1	//write loopback data for basic testing
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
			if (strcmp("--pyscale", argv[i]) == 0)
			{
				if (argc > (i + 1))
				{
					float value = strtof(argv[i + 1], &tmp);
					popts->parser_yscale = value;
					sprintf_s(printstr, sizeof(printstr), "Overriding console print yscale as %f\r\n", popts->parser_yscale);
				}
				else
				{
					sprintf_s(printstr, sizeof(printstr), "invalid yscale format\r\n");
				}
			}
			if (strcmp("--parsermod", argv[i]) == 0)
			{
				if (argc > (i + 1))
				{
					popts->print_in_parser_every_n = (int)strtol(argv[i + 1], &tmp, 10);
					sprintf_s(printstr, sizeof(printstr), "Overriding console print yscale as %d\r\n", popts->print_in_parser_every_n);
				}
				else
				{
					sprintf_s(printstr, sizeof(printstr), "invalid yscale format\r\n");
				}
			}
			if (strcmp("--nwidth", argv[i]) == 0)
			{
				if (argc > (i + 1))
				{
					int value = (int)strtol(argv[i + 1], &tmp, 10);
					if (value > 0 && value < 10000)
					{
						popts->num_widths = value;
						sprintf_s(printstr, sizeof(printstr), "Overriding width arg to %d\r\n", popts->num_widths);
					}
					else
					{
						sprintf_s(printstr, sizeof(printstr), "error: value out of bounds\r\n");
					}
				}
				else
				{
					sprintf_s(printstr, sizeof(printstr), "invalid yscale format\r\n");
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
			if (strcmp("--loopback", argv[i]) == 0)
			{
				popts->write_dummy_loopback = 1;
				sprintf_s(printstr, sizeof(printstr), "Writing Dummy Data...\r\n");
			}

			if (popts->csv_header == 0)
			{
				printf(printstr);
				memset(printstr, 0, sizeof(printstr));
			}
		}
	}
}
