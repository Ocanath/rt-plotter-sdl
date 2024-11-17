/*This source code copyrighted by Lazy Foo' Productions 2004-2023
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL.h>
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

static uint64_t dummy_loopback_txts = 0;

/*
Generic hex checksum calculation.
TODO: use this in the psyonic API
*/
uint32_t fletchers_checksum32(uint32_t* arr, int size)
{
	int32_t checksum = 0;
	int32_t fchk = 0;
	for (int i = 0; i < size; i++)
	{
		checksum += (int32_t)arr[i];
		fchk += checksum;
	}
	return fchk;
}


/*
* Inputs:
*	input_buf: raw unstuffed data buffer
* Outputs:
*	parsed_data: floats, parsed from input buffer
* Returns: number of parsed values
*/
void parse_PPP_values_noscale(uint8_t* input_buf, int payload_size, float* parsed_data, int * parsed_data_size)
{
	uint32_t* pbu32 = (uint32_t*)(&input_buf[0]);
	int32_t* pbi32 = (int32_t*)(&input_buf[0]);
	int wordsize = payload_size / sizeof(uint32_t);
	int i = 0;
	for (i = 0; i < wordsize; i++)
	{
		parsed_data[i] = ((float)pbi32[i]);
		//printf("%d ", pbi32[i]);
	}
	//printf("\n");
	//parsed_data[i] = ((float)pbu32[i]) / 1000.f;

	*parsed_data_size = wordsize;
}


/*
* Inputs:
*	input_buf: raw unstuffed data buffer
* Outputs:
*	parsed_data: floats, parsed from input buffer
* Returns: number of parsed values
*/
void parse_PPP_values(uint8_t* input_buf, int payload_size, float* parsed_data, int * parsed_data_size)
{
	uint32_t* pbu32 = (uint32_t*)(&input_buf[0]);
	int32_t* pbi32 = (int32_t*)(&input_buf[0]);
	int wordsize = payload_size / sizeof(uint32_t);
	int i = 0;
	for (i = 0; i < wordsize - 1; i++)
	{
		parsed_data[i] = ((float)pbi32[i]);
		//printf("%d ", pbi32[i]);
	}
	//printf("\n");
	parsed_data[i] = ((float)pbu32[i]) / 1000.f;

	*parsed_data_size = wordsize;
}


void parse_abh_pkt(uint8_t* pkt, int size)
{
	//for (int pldidx = 0; pldidx < size; pldidx++)
	//	printf("%0.2X", gl_ppp_unstuffing_buffer[pldidx]);
	//printf("\n");
	uint8_t format_header = pkt[0];
	int16_t * ppos16 = (int16_t*)(&pkt[1]);
	for (int i = 0; i < 6; i++)
	{
		double posDeg = ((double)ppos16[i] * 150.) / 32767.;
		printf("%f, ", posDeg);
	}
	printf("\n");
	
}

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



void write_dummy_loopback(HANDLE*pSer)
{
	uint64_t tick = SDL_GetTicks64();
	if (tick - dummy_loopback_txts > 5)
	{
		dummy_loopback_txts = tick;
		//double time = (double)tick / 1000.f;
		//double s1 = sin(time);
		//double c1 = cos(time);

		//int32_t vals[3] = { 0 };
		//uint8_t stuff_buf[sizeof(vals) * 2 + 2] = { 0 };
		//int idx = 0;
		//vals[idx++] = (int32_t)(s1 * 4096.);
		//vals[idx++] = (int32_t)(c1 * 4096.);
		//vals[idx++] = tick;
		//int num_bytes = PPP_stuff((uint8_t*)vals, sizeof(int32_t) * idx, stuff_buf, sizeof(stuff_buf));
		//LPDWORD written = 0;
		//int wfrc = WriteFile(*pSer, stuff_buf, num_bytes, written, NULL);
		uint8_t api_buf[] = { 0x50, 0xA0, 0x00 };	//api/extended mode
		uint8_t stuff_buf[sizeof(api_buf) * 2 + 2] = { 0 };
		api_buf[2] = get_checksum(api_buf, 2);
		int nb = PPP_stuff(api_buf, sizeof(api_buf), stuff_buf, sizeof(stuff_buf));
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
			write_dummy_loopback(pSer);
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
				parse_abh_pkt(gl_ppp_unstuffing_buffer, pld_size);
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
	if (gl_options.print_only)
	{
		text_only(&serialport);
	}

	//The window we'll be rendering to
	SDL_Window* window = NULL;

	SDL_Color bgColor = { 10, 10, 10, 255 };

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	else
	{
		//Create window
		window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (window == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}
		else
		{
			SDL_Renderer* pRenderer = SDL_CreateRenderer(window, -1, 0);
			SDL_SetRenderDrawColor(pRenderer, 255, 255, 255, 255);

			SDL_Event e; 
			bool quit = false; 
			int inc = 0;
			const int dbufsize = SCREEN_WIDTH*gl_options.num_widths;
			std::vector<SDL_Point> points(dbufsize);
			const int numlines = 1;
			std::vector<std::vector<fpoint_t>> fpoints_lines(numlines, std::vector<fpoint_t>(dbufsize) );


			float xscale = 0.f;

			uint64_t start_tick = SDL_GetTicks64();
			uint8_t serialbuffer[10] = { 0 };
			int pld_size = 0;


			int wordsize = 0;
			int previous_wordsize = 0;
			int wordsize_match_count = 0;
			int cycle_count_for_printing = 0;
			uint32_t view_size = 0;
			while (quit == false) 
			{
				uint64_t tick = SDL_GetTicks64() - start_tick;
				float t = ((float)tick) * 0.001f;

				SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
				SDL_RenderClear(pRenderer);

				if (gl_options.write_dummy_loopback)
				{
					write_dummy_loopback(&serialport);
				}

				LPDWORD num_bytes_read = 0;
				pld_size = 0;
				int rc = ReadFile(serialport, gl_ser_readbuf, 512, (LPDWORD)(&num_bytes_read), NULL);	//should be a DOUBLE BUFFER!
				for (int i = 0; i < (int)num_bytes_read; i++)
				{
					uint8_t new_byte = gl_ser_readbuf[i];
					pld_size = parse_PPP_stream(new_byte, gl_ppp_payload_buffer, PAYLOAD_SIZE, gl_ppp_unstuffing_buffer, UNSTUFFING_BUFFER_SIZE, &gl_ppp_bidx);
					if (pld_size > 0)
					{
						if (gl_options.xy_mode == 0)
						{
							parse_PPP_values(gl_ppp_payload_buffer, pld_size, gl_valdump, &wordsize);
						}
						else
						{
							parse_PPP_values_noscale(gl_ppp_payload_buffer, pld_size, gl_valdump, &wordsize);
						}
						
						//obtain consecutive matching counts
						if (wordsize == previous_wordsize && wordsize > 0)
						{
							wordsize_match_count++;
							if (gl_options.print_vals)
							{
								cycle_count_for_printing = (cycle_count_for_printing + 1) % gl_options.print_in_parser_every_n;
								if (cycle_count_for_printing == 0)
								{
									for (int fvidx = 0; fvidx < wordsize; fvidx++)
									{
										printf("%f, ", gl_valdump[fvidx] * gl_options.parser_yscale);
									}
									printf("\n");
								}
							}
						}
						else
						{
							wordsize_match_count = 0;
						}
						previous_wordsize = wordsize;
						

						//resize action
						if(wordsize_match_count >= 100)
						{
							int num_lines = 0;
							if (gl_options.xy_mode == 0)
							{
								num_lines = (wordsize - 1);
							}
							else
							{
								num_lines = wordsize / 2;
							}
							if (fpoints_lines.size() != num_lines)
							{
								fpoints_lines.resize(num_lines, std::vector<fpoint_t>(dbufsize));
							}

						}
					}
				}

				{	//obtain a new xscale.
					//float mintime = 10000000000000.f;
					//float maxtime = 0.f;
					//for (int line = 0; line < fpoints_lines.size(); line++)
					//{
					//	float max_candidate = fpoints_lines[line][dbufsize - 1].x;
					//	float min_candidate = fpoints_lines[line][0].x;
					//	if (max_candidate > maxtime)
					//		maxtime = max_candidate;
					//	if (min_candidate < mintime)
					//		mintime = min_candidate;
					//}
					if (gl_options.xy_mode == 0)
					{
						xscale = ((float)SCREEN_WIDTH) / (fpoints_lines[0][dbufsize - 1].x - fpoints_lines[0][0].x);
					}
				}


				for (int line = 0; line < fpoints_lines.size(); line++)
				{	
					SDL_SetRenderDrawColor(pRenderer, template_colors[line % NUM_COLORS].r, template_colors[line % NUM_COLORS].g, template_colors[line % NUM_COLORS].b, 255);

					//retrieve and load all available datapoints here
					std::vector<fpoint_t>* pFpoints = &fpoints_lines[line];


					float x, y;
					if (gl_options.xy_mode == 0)
					{
						/*Parsing and loading done HERE.
						* if there is a more complex parsing function, implement it elsewhere and have it return X and Y.
						*
						* It should be a function whose input is the unstuffed PPP buffer and whose output is x and y of each line contained in the buffer payload
						*/
						x = gl_valdump[fpoints_lines.size()];
						y = gl_valdump[line];
					}
					else
					{
						x = gl_valdump[line * 2];
						y = gl_valdump[(line * 2) + 1];
					}
					
					std::rotate(pFpoints->begin(), pFpoints->begin() + 1, pFpoints->end());
					(*pFpoints)[dbufsize - 1].x = x;
					(*pFpoints)[dbufsize - 1].y = y;
					if (view_size < dbufsize)
					{
						view_size = view_size + 1;
					}

					float div_pixel_size = 0;
					float div_center = 0;
					if(gl_options.spread_lines)
						div_pixel_size = (float)SCREEN_HEIGHT / ((float)fpoints_lines.size());
					else
					{
						div_center = (float)SCREEN_HEIGHT / 2;
					}

					for (int i = 0; i < points.size(); i++)
					{
						if (gl_options.spread_lines)
						{
							div_center = (div_pixel_size * line) + (div_pixel_size * .5f);	//calculate the center point of the line we're drawing on screen
						}
						if (gl_options.xy_mode == 0)
						{
							points[i].x = (int)(((*pFpoints)[i].x - (*pFpoints)[0].x) * xscale);
							points[i].y = SCREEN_HEIGHT - ((int)((*pFpoints)[i].y * gl_options.yscale) + div_center);
						}
						else
						{
							points[i].x = (int)(((*pFpoints)[i].x) * gl_options.yscale) + SCREEN_WIDTH /  2;
							points[i].y = (-(int)((*pFpoints)[i].y * gl_options.yscale)) + SCREEN_HEIGHT / 2;	//apply uniform scaling
						}
					}

					SDL_RenderDrawLines(pRenderer, (SDL_Point*)(&points[0]), view_size);
				}
				SDL_RenderPresent(pRenderer);
				
				SDL_PollEvent(&e);
				{ 
					if (e.type == SDL_QUIT) 
						quit = true; 
				} 
			}

			//erase all the buffers
			for (int i = 0; i < fpoints_lines.size(); i++)
			{
				fpoints_lines[i].clear();
			}
			fpoints_lines.clear();
			points.clear();

			SDL_DestroyRenderer(pRenderer);
		}
	}

	//Destroy window
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();

	//close serial port
	CloseHandle(serialport);
	return 0;
}
