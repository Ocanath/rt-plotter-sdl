#include <iostream>
#include <stdio.h>
#include <vector>
#include <math.h>
#include "SDL2/SDL.h"


typedef struct fpoint_t
{
	float x;
	float y;
}fpoint_t;

#define NUM_COLORS 8
typedef struct rgb_t
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
}rgb_t;
const rgb_t template_colors[NUM_COLORS] = 
{
	{0xFF, 0xFF, 0xFF},	//white
	{0x00, 0x72, 0xBD},	//blue
	{0xD9, 0x53, 0x19},	//orange
	{0xED, 0xB1, 0x20},	//yellow
	{0x7E, 0x2f, 0x8E},	//purple
	{0x77, 0xAC, 0x30},	//greeeen
	{0x4D, 0xBE, 0xEE},	//light blue
	{0xA2, 0x14, 0x2F}	//maroon
};


//Screen dimension constants
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;


int main(int argc, char *args[]){


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

			while (quit == false) 
			{
				uint64_t tick = SDL_GetTicks64() - start_tick;
				float t = ((float)tick) * 0.001f;

				SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
				SDL_RenderClear(pRenderer);



				for (int line = 0; line < fpoints_lines.size(); line++)
				{	
					SDL_SetRenderDrawColor(pRenderer, template_colors[line % NUM_COLORS].r, template_colors[line % NUM_COLORS].g, template_colors[line % NUM_COLORS].b, 255);

					//retrieve and load all available datapoints here
					std::vector<fpoint_t>* pFpoints = &fpoints_lines[line];

					for (int i = 0; i < points.size(); i++)
					{
						{
							points[i].x = (int)(((*pFpoints)[i].x)) + SCREEN_WIDTH /  2;
							points[i].y = (-(int)((*pFpoints)[i].y)) + SCREEN_HEIGHT / 2;	//apply uniform scaling
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
	
	return 0;
}