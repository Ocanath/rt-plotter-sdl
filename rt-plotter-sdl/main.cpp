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
#include <SDL_ttf.h>


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
void parse_PPP_values_noscale(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size)
{
	uint32_t* pbu32 = (uint32_t*)(&input_buf[0]);
	int32_t* pbi32 = (int32_t*)(&input_buf[0]);
	int wordsize = payload_size / sizeof(uint32_t);
	int i = 0;
	for (i = 0; i < wordsize; i++)
	{
		parsed_data[i] = ((float)pbi32[i]) * gl_options.yscale;
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



void text_only(HANDLE*pSer)
{
	int pld_size = 0;
	int previous_wordsize = 0;
	int wordsize = 0;
	int wordsize_match_count = 0;
	while (1)
	{
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

				//obtain consecutive matching counts
				if (wordsize == previous_wordsize && wordsize > 0)
				{
					wordsize_match_count++;
					for (int fvidx = 0; fvidx < wordsize; fvidx++)
					{
						if (gl_options.print_in_parser == 0)
						{
							float val = gl_valdump[fvidx];
							if (val >= 0)
								printf("+%0.6f", val);
							else
								printf("%0.6f", val);
							
							if (fvidx <  (wordsize - 1))
								printf(", ");
						}
					}
					printf("\n");
				}
				else
				{
					wordsize_match_count = 0;
				}
				previous_wordsize = wordsize;
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
		printf("No COM ports found\n");
	}
	if (gl_options.print_only)
	{
		text_only(&serialport);
	}

	//The window we'll be rendering to
	SDL_Window* window = NULL;

	if (TTF_Init() < 0)
	{
		printf("TTL init failed\n");
	}
	TTF_Font* font = TTF_OpenFont("ARIAL.TTF", 12);
	SDL_Color txt_fgColor = { 255, 255, 255, 255 };
	SDL_Color bgColor = { 10, 10, 10, 255 };
	SDL_Surface* textSurface = TTF_RenderText_Shaded(font, "Text", txt_fgColor, bgColor);

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
			//const int dbufsize = SCREEN_WIDTH*3;
			const int dbufsize = 1000;
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
			std::vector<uint8_t> readbuf;
			int16_t x[3] = { 0 };
			int16_t y[3] = { 0 };
			int16_t V[3] = { 0 };
			int16_t pixRes[3] = { 0 };
			int16_t* pArr[4] = { x, y, V, pixRes };


			uint8_t Multi_Target_Detection_CMD[] = { 0xFD,0xFC,0xFB,0xFA,0x02,0x00,0x90,0x00,0x04,0x03,0x02,0x01 };
			int num_bytes_written = 0;
			WriteFile(serialport, Multi_Target_Detection_CMD, sizeof(Multi_Target_Detection_CMD), (LPDWORD)(&num_bytes_written), NULL);
			//printf("wrote %d words\r\n", num_bytes_written);

			uint64_t send_mt_tracking_ts = 0;
			while (quit == false) 
			{
				uint64_t tick = SDL_GetTicks64() - start_tick;
				float t = ((float)tick) * 0.001f;

				if ((tick - send_mt_tracking_ts) > 300)
				{
					send_mt_tracking_ts = tick;
					WriteFile(serialport, Multi_Target_Detection_CMD, sizeof(Multi_Target_Detection_CMD), (LPDWORD)(&num_bytes_written), NULL);
					printf("wrote %d words\r\n", num_bytes_written);
				}

				SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
				SDL_RenderClear(pRenderer);



				LPDWORD num_bytes_read = 0;
				pld_size = 0;
				int rc = ReadFile(serialport, gl_ser_readbuf, 512, (LPDWORD)(&num_bytes_read), NULL);	//should be a DOUBLE BUFFER!
				for (int i = 0; i < (int)num_bytes_read; i++)
				{
					for (int i = 0; i < (int)num_bytes_read; i++)
					{
						uint8_t new_byte = gl_ser_readbuf[i];
						readbuf.push_back(new_byte);
					}
					for (int i = 0; i < ((int)readbuf.size()) - 1; i++)
					{
						if (readbuf[i] == 0x55 && readbuf[i+1] == 0xCC)
						{

							//printf("%d\r\n", i);
							int startidx = i - 28;
							if (startidx >= 0)
							{
								//printf("%X%X%X%0.2X: ", readbuf[startidx], readbuf[startidx+1], readbuf[startidx+2], readbuf[startidx+3]);
								int bufidx = startidx + 4;
								for (int target = 0; target < 3; target++)
								{
									for (int arridx = 0; arridx < 4; arridx++)
									{
										int32_t tmp = 0;
										uint8_t b1 = readbuf[bufidx];
										uint8_t b2 = readbuf[bufidx+1];
										tmp = (int32_t)b1 + ((int32_t)b2 * 256);
										if (tmp < 0x8000)
											pArr[arridx][target] = (int16_t)(0 - tmp);
										else
											pArr[arridx][target] = (int16_t)(tmp - 0x8000);
										bufidx += 2;
									}
									printf("(%d,%d,%d) ", x[target], y[target], V[target]);
								}
								printf("\r\n");
							}
							readbuf.clear();
						}
					}
				}

				for(int target = 0; target < 3; target++)
				{	
					SDL_SetRenderDrawColor(pRenderer, template_colors[target].r, template_colors[target].g, template_colors[target].b, 255);

					const float target_radius_pixels = 50.f;
					float increment = (2 * 3.14159265f) / ((float)points.size());
					for (int i = 0; i < points.size(); i++)
					{
						float t_parametric = increment * (float)i;

						points[i].x = (int)(cos(t_parametric) * target_radius_pixels) + SCREEN_WIDTH / 2 + (int)((float)x[target] * 0.3f);
						points[i].y = (int)(sin(t_parametric)* target_radius_pixels) + (int)((float)y[target] * 0.3f);
					}

					SDL_RenderDrawLines(pRenderer, (SDL_Point*)(&points[0]), dbufsize);
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
