/*This source code copyrighted by Lazy Foo' Productions 2004-2023
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <stdio.h>
#include <math.h>
#include <vector>
#include "winserial.h"
#include "PPP.h"
#include "colors.h"
#include "args-parsing.h"
#include <algorithm>

#define PAYLOAD_SIZE 512
#define UNSTUFFING_BUFFER_SIZE (PAYLOAD_SIZE * 2 + 2)

//Screen dimension constants
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;

typedef struct fpoint_t
{
	float x;
	float y;
}fpoint_t;

static int gl_ppp_bidx = 0;
static uint8_t gl_ppp_payload_buffer[PAYLOAD_SIZE] = { 0 };	//buffer
static uint8_t gl_ppp_unstuffing_buffer[UNSTUFFING_BUFFER_SIZE] = { 0 };
static uint8_t gl_ser_readbuf[512] = { 0 };
static float gl_valdump[PAYLOAD_SIZE / sizeof(float)] = { 0 };


/*
Generic hex checksum calculation.
TODO: use this in the psyonic API
*/
uint8_t get_checksum(uint8_t* arr, int size)
{

	int8_t checksum = 0;
	for (int i = 0; i < size; i++)
		checksum += (int8_t)arr[i];
	return -checksum;
}


/*
* Inputs:
*	input_buf: raw unstuffed data buffer
* Outputs:
*	parsed_data: floats, parsed from input buffer
* Returns: number of parsed values
*/
void parse_PPP_values_noscale(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size)
{
	uint8_t vibration = input_buf[1];
	uint8_t dc_state = input_buf[9]; 
	uint8_t r = input_buf[4];
	uint8_t g = input_buf[5];
	uint8_t b = input_buf[6];
	uint8_t chk = get_checksum(input_buf, 10);
	if (chk == input_buf[10])
	{
		const char* vsig_map[6] = { "DIFF_NEG", "DIFF_POS", "CO_CONTRACT", "RELAX_SIG", "PULSE_POS", "PULSE_NEG" };
		if (dc_state >= 0 && dc_state < 6)
		{
			printf("Vib = %d, color = %0.2X%0.2X%0.2X, Sig = %s\n", vibration, r, g, b, vsig_map[dc_state]);
		}
		else
		{
			printf("error, dc state out of bounds\n");
		}
	}
	else
	{
		printf("Checksum Mismatch\n");
	}
}


uint64_t gl_txts = 0;
void write_hand_command(HANDLE*pSer)
{
	uint64_t tick = GetTickCount64();
	if (tick - gl_txts > 10)
	{
		gl_txts = tick;

		uint8_t buf[] = { 0x50, 0x48, 0x00 };
		uint8_t stuff_buf[sizeof(buf) * 2 + 2] = { 0 };

		buf[2] = get_checksum(buf, 2);
		int nb = PPP_stuff(buf, sizeof(buf), stuff_buf, sizeof(stuff_buf));
		LPDWORD written = 0;
		int wfrc = WriteFile(*pSer, stuff_buf, nb, written, NULL);
	}
}

void text_only(HANDLE*pSer)
{
	int pld_size = 0;
	int previous_wordsize = 0;
	int wordsize = 0;
	int wordsize_match_count = 0;
	
	while (1)
	{
		if (gl_options.write_dummy_loopback)
		{
			write_hand_command(pSer);
		}
		LPDWORD num_bytes_read = 0;
		pld_size = 0;
		int rc = ReadFile(*pSer, gl_ser_readbuf, 512, (LPDWORD)(&num_bytes_read), NULL);	//should be a DOUBLE BUFFER!
		for (int i = 0; i < (int)num_bytes_read; i++)
		{
			uint8_t new_byte = gl_ser_readbuf[i];
			pld_size = parse_PPP_stream(new_byte, gl_ppp_payload_buffer, PAYLOAD_SIZE, gl_ppp_unstuffing_buffer, UNSTUFFING_BUFFER_SIZE, &gl_ppp_bidx);
			if (pld_size > 0)
			{
				parse_PPP_values_noscale(gl_ppp_payload_buffer, pld_size, gl_valdump, &wordsize);
			}
		}
	}
}


int main(int argc, char* args[])
{
	parse_args(argc, args, &gl_options);

	HANDLE serialport;
	char namestr[16] = { 0 };
	uint8_t found = 0;
	for (int i = 0; i < 255; i++)
	{
		int rl = sprintf_s(namestr, "\\\\.\\COM%d", i);
		int rc = connect_to_usb_serial(&serialport, namestr, gl_options.baud_rate);
		if (rc != 0)
		{
			if (!(gl_options.csv_header == 1 && (gl_options.print_only == 1 || gl_options.print_in_parser == 1)))
			{
				printf("Connected to COM port %s successfully\n", namestr);
			}
			found = 1;
			break;
		}
	}
	if (found == 0)
	{
		if(gl_options.csv_header == 0)
			printf("No COM ports found\n");
	}

	text_only(&serialport);

	//close serial port
	CloseHandle(serialport);
	return 0;
}
