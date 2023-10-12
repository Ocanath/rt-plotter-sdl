/*This source code copyrighted by Lazy Foo' Productions 2004-2023
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include "winserial.h"

//Screen dimension constants
const int SCREEN_WIDTH = 1040;
const int SCREEN_HEIGHT = 480;

typedef struct fpoint_t
{
	float x;
	float y;
}fpoint_t;

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
			std::vector<fpoint_t> fpoints(dbufsize);


			float xscale = 0.f;
			float yscale = 100 / 1.f;	//100 pixels = 1 input unit

			uint64_t start_tick = SDL_GetTicks64();
			
			while (quit == false) 
			{
				uint64_t tick = SDL_GetTicks64() - start_tick;
				float t = ((float)tick) * 0.001f;

				SDL_SetRenderDrawColor(pRenderer, 10, 10, 10, 255);
				SDL_RenderClear(pRenderer);

				SDL_SetRenderDrawColor(pRenderer, 255, 255, 255, 255);

				float x = t;
				float y = sin((double)(t * 2.f * 3.141595f * 1.f));
				std::rotate(fpoints.begin(), fpoints.begin() + 1, fpoints.end());
				fpoints[dbufsize - 1].x = t;
				fpoints[dbufsize - 1].y = y;
				xscale = ((float)SCREEN_WIDTH)/(fpoints[dbufsize - 1].x - fpoints[0].x);

				for (int i = 0; i < points.size(); i++)
				{
					points[i].x = (int)( (fpoints[i].x - fpoints[0].x) * xscale);
					points[i].y = (int)(fpoints[i].y * yscale) + SCREEN_HEIGHT / 2;
				}

				SDL_RenderDrawLines(pRenderer, (SDL_Point*)(&points[0]), dbufsize);

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
