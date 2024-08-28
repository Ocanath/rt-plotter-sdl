#include <iostream>
#include <stdio.h>
#include <vector>
#include <math.h>
#include <algorithm>
#include "SDL2/SDL.h"
#include "PPP.h"
#include "colors.h"
#include "args-parsing.h"
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


void text_only(int serial_handle)
{
	int pld_size = 0;
	int previous_wordsize = 0;
	int wordsize = 0;
	int wordsize_match_count = 0;
	while (1)
	{
		int nb = read(serial_handle, &gl_ser_readbuf, sizeof(gl_ser_readbuf) );
		if(nb > 0)
		{
			for (int i = 0; i < nb; i++)
			{
				uint8_t new_byte = gl_ser_readbuf[i];
				pld_size = parse_PPP_stream(new_byte, gl_ppp_payload_buffer, PAYLOAD_SIZE, gl_ppp_unstuffing_buffer, UNSTUFFING_BUFFER_SIZE, &gl_ppp_bidx);
				if (pld_size > 0)
				{
					printf("received %d bytes: ", pld_size);
					for(int bv = 0; bv < pld_size; bv++)
					{
						printf("%c",gl_ppp_payload_buffer[bv]);
					}
					printf("\r\n");
					// parse_PPP_values_noscale(gl_ppp_payload_buffer, pld_size, gl_valdump, &wordsize);

					// //obtain consecutive matching counts
					// if (wordsize == previous_wordsize && wordsize > 0)
					// {
					// 	wordsize_match_count++;
					// 	for (int fvidx = 0; fvidx < wordsize; fvidx++)
					// 	{
					// 		if (gl_options.print_in_parser == 0)
					// 		{
					// 			float val = gl_valdump[fvidx]*gl_options.yscale;
					// 			if (val >= 0)
					// 				printf("+%0.6f", val);
					// 			else
					// 				printf("%0.6f", val);
								
					// 			if (fvidx <  (wordsize - 1))
					// 				printf(", ");
					// 		}
					// 	}
					// 	printf("\n");
					// }
					// else
					// {
					// 	wordsize_match_count = 0;
					// }
					// previous_wordsize = wordsize;
				}
			}
		}
	}
}



//Screen dimension constants
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;


int main(int argc, char *args[])
{

	parse_args(argc, args, &gl_options);
	
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
	tty.c_ispeed = gl_options.baud_rate;
	tty.c_ospeed = gl_options.baud_rate;

	//timeout=0
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 0;

	ioctl(serial_port, TCSETS2, &tty);

	if (gl_options.print_only)
	{
		text_only(serial_port);
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
				int nb = read(serial_port, &gl_ser_readbuf, sizeof(gl_ser_readbuf) );
				for(int i = 0; i < nb; i++)
				{
					uint8_t new_byte = gl_ser_readbuf[i];
					int pld_size = parse_PPP_stream(new_byte, gl_ppp_payload_buffer, PAYLOAD_SIZE, gl_ppp_unstuffing_buffer, UNSTUFFING_BUFFER_SIZE, &gl_ppp_bidx);
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
								for (int fvidx = 0; fvidx < wordsize; fvidx++)
								{
									printf("%f, ", gl_valdump[fvidx]*gl_options.yscale);
								}
								printf("\n");
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

	close(serial_port);
	
	return 0;
}