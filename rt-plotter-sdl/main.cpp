/*This source code copyrighted by Lazy Foo' Productions 2004-2023
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include "winserial.h"
#include "PPP.h"

#define PAYLOAD_SIZE 512
#define UNSTUFFING_BUFFER_SIZE (PAYLOAD_SIZE * 2 + 2)

//Screen dimension constants
const int SCREEN_WIDTH = 1040;
const int SCREEN_HEIGHT = 480;



typedef struct fpoint_t
{
	float x;
	float y;
}fpoint_t;

static int gl_ppp_bidx = 0;
static uint8_t gl_ppp_payload_buffer[PAYLOAD_SIZE] = { 0 };	//buffer
static uint8_t gl_ppp_unstuffing_buffer[UNSTUFFING_BUFFER_SIZE] = { 0 };


int main(int argc, char* args[])
{
	
	HANDLE serialport;
	char namestr[16] = { 0 };
	uint8_t found = 0;
	for (int i = 0; i < 255; i++)
	{
		int rl = sprintf_s(namestr, "\\\\.\\COM%d", i);
		int rc = connect_to_usb_serial(&serialport, namestr, 460800);
		if (rc != 0)
		{
			printf("Connected to COM port %s successfully\r\n", namestr);
			found = 1;
			break;
		}
	}
	if (found == 0)
	{
		printf("No COM ports found\r\n");
	}

	uint8_t ser_readbuf[512] = { 0 };
	
	while (1)
	{
		LPDWORD num_bytes_read = 0;
		int rc = ReadFile(serialport, ser_readbuf, 512, (LPDWORD)(&num_bytes_read), NULL);	//should be a DOUBLE BUFFER!
		for (int i = 0; i < (int)num_bytes_read; i++)
		{
			
			printf("read %d bytes: %c\r\n", num_bytes_read, ser_readbuf[i]);
		}
	}


	//The window we'll be rendering to
	SDL_Window* window = NULL;

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
			const int dbufsize = SCREEN_WIDTH;
			std::vector<SDL_Point> points(dbufsize);
			const int numlines = 3;
			std::vector<std::vector<fpoint_t>> fpoints_lines(numlines, std::vector<fpoint_t>(dbufsize) );


			float xscale = 0.f;
			float yscale = 100 / 1.f;	//100 pixels = 1 input unit

			uint64_t start_tick = SDL_GetTicks64();
			uint8_t serialbuffer[10] = { 0 };

			while (quit == false) 
			{
				uint64_t tick = SDL_GetTicks64() - start_tick;
				float t = ((float)tick) * 0.001f;

				SDL_SetRenderDrawColor(pRenderer, 10, 10, 10, 255);
				SDL_RenderClear(pRenderer);

				SDL_SetRenderDrawColor(pRenderer, 255, 255, 255, 255);

				int recievedsize = 0;
				for(int i = 0; i < recievedsize; i++)
				{
					uint8_t new_byte = serialbuffer[i];
					int pld_size = parse_PPP_stream(new_byte, gl_ppp_payload_buffer, PAYLOAD_SIZE, gl_ppp_unstuffing_buffer, UNSTUFFING_BUFFER_SIZE, &gl_ppp_bidx);
					if (pld_size > 0)
					{
						printf("%s\r\n", gl_ppp_payload_buffer);
					}
				}


				for (int line = 0; line < fpoints_lines.size(); line++)
				{	//retrieve and load all available datapoints here
					std::vector<fpoint_t>* pFpoints = &fpoints_lines[line];

					float x = t;
					float y = sin((double)(t * 2.f * 3.141595f * 1.f));

					std::rotate(pFpoints->begin(), pFpoints->begin() + 1, pFpoints->end());
					(*pFpoints)[dbufsize - 1].x = t;
					(*pFpoints)[dbufsize - 1].y = y;
					xscale = ((float)SCREEN_WIDTH) / ((*pFpoints)[dbufsize - 1].x - (*pFpoints)[0].x);


					for (int i = 0; i < points.size(); i++)
					{
						points[i].x = (int)(((*pFpoints)[i].x - (*pFpoints)[0].x) * xscale);
						points[i].y = (int)((*pFpoints)[i].y * yscale) + (line+1)*(float)SCREEN_HEIGHT / (float)(fpoints_lines.size()+1) ;
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
			SDL_DestroyRenderer(pRenderer);
		}
	}

	//Destroy window
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}
