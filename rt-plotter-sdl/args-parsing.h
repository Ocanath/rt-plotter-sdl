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
}cmd_options_t;

extern cmd_options_t gl_options;


typedef enum {
	INTEGER_TYPE,
	FLOAT32_TYPE
}parse_types_t;

void parse_args(int argc, char* argv[], cmd_options_t* popts);


#endif