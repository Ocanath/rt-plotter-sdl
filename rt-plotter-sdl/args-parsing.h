#ifndef ARGS_PARSING_H
#define ARGS_PARSING_H
#include <stdint.h>
#include "u32_fmt_t.h"

typedef struct cmd_options_t
{
	uint8_t spread_lines;
	int baud_rate;
	float yscale;
	uint8_t print_vals;
	uint8_t print_only;
	uint8_t print_in_parser;
	uint8_t xy_mode;	//option to enable xy mode. In this mode, the data stream is mapped as value pairs (0-1, 2-3, etc.) where 0 is x, 1 is y. 
	uint8_t csv_header;
}cmd_options_t;

extern cmd_options_t gl_options;


typedef enum {
	INTEGER_TYPE,
	FLOAT32_TYPE
}parse_types_t;

void parse_args(int argc, char* argv[], cmd_options_t* popts);


#endif