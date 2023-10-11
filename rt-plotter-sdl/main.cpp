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

	//The surface contained by the window
	SDL_Surface* screenSurface = NULL;

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
			//Get window surface
			screenSurface = SDL_GetWindowSurface(window);
			SDL_Renderer* pRenderer = SDL_CreateRenderer(window, -1, 0);
			SDL_SetRenderDrawColor(pRenderer, 255, 255, 255, 255);

			//Fill the surface white
			SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 10, 10, 10));

			//Update the surface
			SDL_UpdateWindowSurface(window);

			//Hack to get window to stay up
			SDL_Event e; 
			bool quit = false; 
			int inc = 0;
			while (quit == false) 
			{
				SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 10, 10, 10));

				uint64_t tick = SDL_GetTicks64();
				float t = ((float)tick) * 0.001f;

				float ft = t * 10;

				SDL_Rect rectangule;
				rectangule.x = SCREEN_WIDTH / 2;
				rectangule.y = SCREEN_HEIGHT / 2;
				rectangule.w = (int)( (sin(ft)*0.5f + 0.5f) * 100)+50;
				rectangule.h = (int)( (cos(ft)*0.5f + 0.5f) * 100)+50;
				SDL_FillRect(screenSurface, &rectangule, SDL_MapRGB(screenSurface->format, 255, 255, 255));
				SDL_UpdateWindowSurface(window);



				SDL_PollEvent(&e);
				{ 
					if (e.type == SDL_QUIT) 
						quit = true; 
				} 
			}
		}
	}

	//Destroy window
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}
