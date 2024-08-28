#include <iostream>
#include <stdio.h>
#include <vector>
#include <math.h>
#include <algorithm>
#include "SDL2/SDL.h"
#include "PPP.h"
#include "colors.h"
//linux headers for serial port
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <unistd.h> // write(), read(), close()
#include <asm/ioctls.h>
#include <asm/termbits.h>
#include <sys/ioctl.h> // Used for TCGETS2, which is required for custom baud rates


typedef struct fpoint_t
{
	float x;
	float y;
}fpoint_t;

#define PAYLOAD_SIZE 512
#define UNSTUFFING_BUFFER_SIZE (PAYLOAD_SIZE * 2 + 2)

static int gl_ppp_bidx = 0;
static uint8_t gl_ppp_payload_buffer[PAYLOAD_SIZE] = { 0 };	//buffer
static uint8_t gl_ppp_unstuffing_buffer[UNSTUFFING_BUFFER_SIZE] = { 0 };
static uint8_t gl_ser_readbuf[512] = { 0 };
static float gl_valdump[PAYLOAD_SIZE / sizeof(float)] = { 0 };


//Screen dimension constants
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;


int main(int argc, char *args[])
{
	int serial_port = open("/dev/ttyUSB0", O_RDWR);
	if (serial_port < 0) {
		printf("Error %i from open: %s\n", errno, strerror(errno));
	}

	struct termios2 tty;
	ioctl(serial_port, TCGETS2, &tty);
	tty.c_cflag     &=  ~CSIZE;		// CSIZE is a mask for the number of bits per character
	tty.c_cflag |= CS8; 			// 8 bits per byte (most common)
	tty.c_cflag &= ~PARENB; 		// Clear parity bit, disabling parity (most common)
	tty.c_cflag     &=  ~CSTOPB;	// one stop bit
	tty.c_cflag &= ~CRTSCTS;		//hw flow control off
	tty.c_cflag     |=  CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
	
	tty.c_oflag     =   0;              // No remapping, no delays
	tty.c_oflag     &=  ~OPOST;         // Make raw
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	tty.c_iflag 	&= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag		&= ~ECHOE;     // Turn off echo erase (echo erase only relevant if canonical input is active)
	tty.c_lflag		&= ~ECHONL;    //
	tty.c_lflag		&= ~ISIG;      // Disables recognition of INTR (interrupt), QUIT and SUSP (suspend) characters


	//custom baud rate
	tty.c_cflag &= ~CBAUD;
	tty.c_cflag |= CBAUDEX;
	// tty.c_cflag |= BOTHER;
	tty.c_ispeed = 921600;
	tty.c_ospeed = 921600;

	//timeout=0
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 0;

	ioctl(serial_port, TCSETS2, &tty);


	uint64_t tick = 0;
	while(tick < 0xFFFFFFFF * 5)
	{
		int nb = read(serial_port, &gl_ser_readbuf, sizeof(gl_ser_readbuf) );
		if(nb > 0)
		{
			printf("%d bytes: ",nb);
			for(int i = 0; i < nb; i++)
			{
				printf("%c", gl_ser_readbuf[i]);
			}
			printf("\r\n");
		}
	}

	close(serial_port);
	return 0;


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
			const int numlines = 2;
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
				double t = ((double)tick) * 0.001;

				SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
				SDL_RenderClear(pRenderer);

				/*
				Get Data over Serial Stream
				*/
				uint8_t new_byte = (uint8_t)t;
				int pld_size = parse_PPP_stream(new_byte, gl_ppp_payload_buffer, PAYLOAD_SIZE, gl_ppp_unstuffing_buffer, UNSTUFFING_BUFFER_SIZE, &gl_ppp_bidx);


				for (int line = 0; line < fpoints_lines.size(); line++)
				{	
					SDL_SetRenderDrawColor(pRenderer, template_colors[line % NUM_COLORS].r, template_colors[line % NUM_COLORS].g, template_colors[line % NUM_COLORS].b, 255);

					//retrieve and load all available datapoints here
					std::vector<fpoint_t>* pFpoints = &fpoints_lines[line];

					std::rotate(pFpoints->begin(), pFpoints->begin() + 1, pFpoints->end());
					(*pFpoints)[dbufsize - 1].x = sin(t)*100 + 100*(line%2);
					(*pFpoints)[dbufsize - 1].y = cos(t)*100;


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