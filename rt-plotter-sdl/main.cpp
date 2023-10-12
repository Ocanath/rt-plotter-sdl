/*This source code copyrighted by Lazy Foo' Productions 2004-2023
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>
#include <math.h>

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main(int argc, char* args[])
{
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
			const int dbufsize = 1000;
			SDL_Point points[dbufsize];
			while (quit == false) 
			{
				uint64_t tick = SDL_GetTicks64();
				float t = ((float)tick) * 0.001f;


				SDL_SetRenderDrawColor(pRenderer, 10, 10, 10, 255);
				SDL_RenderClear(pRenderer);



				SDL_SetRenderDrawColor(pRenderer, 255, 255, 255, 255);


				//float freq = (sin(t) * 0.5f + 0.5f)*0.01f + 0.01f;
				float freq = 0.02f;
				float phase = t;
				for (int i = 0; i < dbufsize; i++)
				{
					points[i].x = i;
					points[i].y = (int)(sin( ((double)i)*freq + phase)*100.f+SCREEN_HEIGHT/2);
				}

				SDL_RenderDrawLines(pRenderer, points, dbufsize);

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
