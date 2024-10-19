/*This source code copyrighted by Lazy Foo' Productions 2004-2023
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>
#include<winsock2.h>
#include <WS2tcpip.h>
#include "WinUdpClient.h"

#include <math.h>
#include <vector>
#include "winserial.h"
#include "PPP.h"
#include "colors.h"
#include "args-parsing.h"
#include <algorithm>
#include "trig_fixed.h"


#pragma comment(lib,"ws2_32.lib") //Winsock Library


enum { POSITION = 0xFA, TURBO = 0xFB, STEALTH = 0xFC };

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
static uint8_t gl_ppp_stuffing_buffer[PAYLOAD_SIZE] = { 0 };
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
Generic hex checksum calculation.
TODO: use this in the psyonic API
*/
uint16_t fletchers_checksum16(uint16_t* arr, int size)
{
	int16_t checksum = 0;
	int16_t fchk = 0;
	for (int i = 0; i < size; i++)
	{
		checksum += (int16_t)arr[i];
		fchk += checksum;
	}
	return fchk;
}


/*Value clamping symmetric about zero*/
int32_t symm_thresh(int32_t sig, int32_t thresh)
{
	if (sig > thresh)
		sig = thresh;
	if (sig < -thresh)
		sig = -thresh;
	return sig;
}

int main(int argc, char* args[])
{
	parse_args(argc, args, &gl_options);

	WinUdpClient client(6701);
	client.set_nonblocking();
	client.si_other.sin_family = AF_INET;
	//client.si_other.sin_addr.S_un.S_addr = inet_addr("192.168.123.255");
	//TODO: auto address discovery with WHO_GOES_THERE
	inet_pton(AF_INET, "192.168.123.86", &client.si_other.sin_addr);

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
			const int dbufsize = SCREEN_WIDTH*3;
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
			SDL_SetRelativeMouseMode(SDL_TRUE);
			int prev_mouse_x = SCREEN_WIDTH / 2;
			int prev_mouse_y = SCREEN_HEIGHT / 2;
			int32_t accum_mouse_x = 0;
			int32_t accum_mouse_y = 0;
			int32_t forward = 0;
			SDL_WarpMouseInWindow(window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
			while (quit == false) 
			{
				uint64_t tick = SDL_GetTicks64() - start_tick;
				float t = ((float)tick) * 0.001f;

				SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
				SDL_RenderClear(pRenderer);

				int mouse_x, mouse_y;
				SDL_GetMouseState(&mouse_x, &mouse_y);
				int32_t gain_x = 10;
				int32_t gain_y = 10;
				accum_mouse_x = wrap_2pi_14b( (accum_mouse_x + (mouse_x - prev_mouse_x)* gain_x) );
				accum_mouse_y = wrap_2pi_14b( ( accum_mouse_y + (mouse_y - prev_mouse_y)* gain_y) );
				accum_mouse_y = symm_thresh(accum_mouse_y, PI_14B / 4);
				int32_t w1 = wrap_2pi_14b(accum_mouse_x + forward);
				int32_t w2 = wrap_2pi_14b(accum_mouse_x - forward);
				int32_t gy = wrap_2pi_14b(accum_mouse_y);
				printf("%f, %f, %f\n", (float)w1*(180.f/(float)PI_14B), (float)w2 * (180.f / (float)PI_14B), (float)gy*(180.f/(float)PI_14B));
				SDL_WarpMouseInWindow(window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

				uint8_t pld[32] = { 0 };
				uint16_t* pu16 = (uint16_t*)(&pld[0]);
				int i = 0;
				pld[i++] = POSITION;
				pld[i++] = 0;
				
				int32_t* p_cmd32 = (int32_t *)(& pld[i]);
				p_cmd32[0] = w1;
				i += sizeof(int32_t);
				p_cmd32[1] = w2;
				i += sizeof(int32_t);
				p_cmd32[2] = gy;
				i += sizeof(int32_t);

				int chkidx = (i + (i % 2)) / 2;	//pad a +1 byte if it's odd, divide by 2, set that as the start of our 16bit checksum
				pu16[chkidx] = fletchers_checksum16(pu16, chkidx);
				chkidx++;
				int pld_size = chkidx * sizeof(uint16_t);
				int stuffed_size = PPP_stuff(pld, pld_size, gl_ppp_stuffing_buffer, sizeof(gl_ppp_stuffing_buffer));
				sendto(client.s, (const char*)gl_ppp_stuffing_buffer, stuffed_size, 0, (struct sockaddr*)&client.si_other, client.slen);


				uint8_t new_pkt = 0;

				if (gl_options.xy_mode == 0)
				{
					xscale = ((float)SCREEN_WIDTH) / (fpoints_lines[0][dbufsize - 1].x - fpoints_lines[0][0].x);
				}

				for (int line = 0; line < fpoints_lines.size(); line++)
				{	
					SDL_SetRenderDrawColor(pRenderer, template_colors[line % NUM_COLORS].r, template_colors[line % NUM_COLORS].g, template_colors[line % NUM_COLORS].b, 255);

					//retrieve and load all available datapoints here
					std::vector<fpoint_t>* pFpoints = &fpoints_lines[line];

					if (new_pkt)
					{
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

	closesocket(client.s);
	WSACleanup();

	return 0;
}
