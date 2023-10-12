#include "args-parsing.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

cmd_options_t gl_options = {
	0,	//spread lines
	921600,	//baud rage
	100.f
};

///*
//* Get integer argument from a command line string
//* INPUTS:
//*	basearg: string which represents the base argument, pre value
//* OUTPUTS:
//* RETURNS:
//*/
//int get_int_arg(const char* basearg, char* arg, parse_types_t type, void * pValue)
//{
//	int blen = strlen(basearg);
//	int arglen = strlen(arg);
//	int res = strncmp(basearg, arg, blen);
//	if (res == 0 && arglen > blen + 1)	//must have space and at least one additional character
//	{
//		if (arg[blen] == ' ')
//		{
//			char* tmp;
//			if (type == INTEGER_TYPE)
//			{
//				int value = strtol(&arg[blen + 1], &tmp, 10);
//				*((int*)pValue) = value;
//			}
//			else if (type == FLOAT32_TYPE)
//			{
//				float value = strtof(&arg[blen + 1], &tmp);
//				*((float*)pValue) = value;
//			}
//			return 0;
//		}
//	}
//	return -1;
//}

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
		}
	}

}
