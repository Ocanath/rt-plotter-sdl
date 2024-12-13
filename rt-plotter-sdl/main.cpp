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
Generic hex checksum calculation.
TODO: use this in the psyonic API
 */
uint16_t get_checksum16(uint16_t* arr, int size)
{
	int16_t checksum = 0;
	for (int i = 0; i < size; i++)
		checksum += (int16_t)arr[i];
	return -checksum;
}


static const float offsets[] = { -2.700014, -1.099270, -1.576392, 1.654050, -2.082925, -0.006566 };
static const float signs[] = { 1,1,1,1,1,1 };

#define ONE_BY_TWO_PI 			0.1591549f
#define TWO_PI              	6.28318530718f
#define PI						3.14159265359f

/*
	fast 2pi mod. needed for sin and cos FAST for angle limiting
 */
float fmod_2pi(float in)
{
	uint8_t aneg = 0;
	float in_eval = in;
	if (in < 0)
	{
		aneg = 1;
		in_eval = -in;
	}
	float fv = (float)((int)(in_eval * ONE_BY_TWO_PI));
	if (aneg == 1)
		fv = (-fv) - 1;
	return in - TWO_PI * fv;
}

/*
	General purpose 2pi wrap. Ensures -pi to pi
 */
float wrap_2pi(float v)
{
	return fmod_2pi(v + PI) - PI;
}

/*
* Inputs:
*	input_buf: raw unstuffed data buffer
* Outputs:
*	parsed_data: floats, parsed from input buffer
* Returns: number of parsed values
*/
void parse_read(uint8_t* input_buf, int input_size, float* parsed_data, int parsed_size)
{
	uint16_t* pbuf16 = (uint16_t*)input_buf;

	uint16_t chk = get_checksum16(pbuf16, 3);
	if (chk == pbuf16[3])
	{
		//printf("Address:%d, cos:%d, sin:%d\n", pbuf16[0], pbuf16[1], pbuf16[2]);
		uint16_t address = pbuf16[0] - 1;	//start everything at 1
		if (address >= 0 && address < parsed_size)
		{
			if (address < (sizeof(offsets) / sizeof(float)) && address < (sizeof(signs) / sizeof(float)))	//bounds check on signs and offsets arrays
			{
				double sin = (double)pbuf16[2] - 1995.;
				double cos = (double)pbuf16[1] - 1995.;
				float angle = wrap_2pi((float)atan2(sin, cos) - offsets[address])*signs[address];
				parsed_data[address] = angle;
			}
		}
		
	}
	//else
	//{
	//	printf("Checksum Mismatch\n");
	//}
}


void write_encoder_command(HANDLE*pSer, uint16_t address)
{
	uint8_t stuff_buf[sizeof(address) * 2 + 2] = { 0 };

	int nb = PPP_stuff((uint8_t*)(&address), sizeof(address), stuff_buf, sizeof(stuff_buf));
	LPDWORD written = 0;
	int wfrc = WriteFile(*pSer, stuff_buf, nb, written, NULL);
}

void main_loop(HANDLE*pSer)
{
	int pld_size = 0;
	int previous_wordsize = 0;
	int wordsize = 0;
	int wordsize_match_count = 0;
	
	uint16_t addresses[] = { 1,2,3,4 ,5, 6};
	int num_addresses = (sizeof(addresses) / sizeof(uint16_t));
	float angles[(sizeof(addresses) / sizeof(uint16_t))] = { 0 };
	int addr_idx = 0;
	uint64_t tx_ts = 0;
	uint8_t done = 0;
	while (1)
	{
		write_encoder_command(pSer, addresses[addr_idx]);
	
		uint8_t poll_for_response = 1;
		uint64_t start_ts = GetTickCount64();
		while (poll_for_response != 0)
		{
			uint64_t tick = GetTickCount64();
			LPDWORD num_bytes_read = 0;
			pld_size = 0;
			int rc = ReadFile(*pSer, gl_ser_readbuf, 512, (LPDWORD)(&num_bytes_read), NULL);	//should be a DOUBLE BUFFER!
			for (int i = 0; i < (int)num_bytes_read; i++)
			{
				uint8_t new_byte = gl_ser_readbuf[i];
				pld_size = parse_PPP_stream(new_byte, gl_ppp_payload_buffer, PAYLOAD_SIZE, gl_ppp_unstuffing_buffer, UNSTUFFING_BUFFER_SIZE, &gl_ppp_bidx);
				if (pld_size > 0)
				{
					parse_read(gl_ppp_payload_buffer, pld_size, angles, num_addresses);
					poll_for_response = 0;

					addr_idx = (addr_idx + 1);
					if (addr_idx >= num_addresses)
					{
						addr_idx = 0;
						done = 1;
					}
					
				}
			}
			if (tick - start_ts > 1)
			{
				poll_for_response = 0;
				addr_idx = (addr_idx + 1);
				if (addr_idx >= num_addresses)
				{
					addr_idx = 0;
					done = 1;
				}

			}
		}


		if(done != 0)
		{
			for (int i = 0; i < num_addresses; i++)
			{
				printf("%.2f, ", angles[i]*180./3.14159265);
			}
			printf("\n");
			done = 0;
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

	main_loop(&serialport);

	//close serial port
	CloseHandle(serialport);
	return 0;
}
