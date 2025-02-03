#include <SDL.h>
#include <stdio.h>
#include <vector>

#ifdef PLATFORM_WINDOWS
#include "winserial.h"
#elif defined(PLATFORM_LINUX)
#include "linux-serial.h"
#endif

#include "PPP.h"
#include "colors.h"
#include "args-parsing.h"
#include <algorithm>
#include "fsrs.h"
#include "ppp-parsing.h"
#include "magsensor.h"

//Screen dimension constants
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;

typedef struct fpoint_t
{
	float x;
	float y;
}fpoint_t;

double gain_res_xy[8][4] = {
	{0.751, 1.502, 3.004, 6.009},
	{0.601, 1.202, 2.403, 4.840},
	{0.451, 0.901, 1.803, 3.605},
	{0.376, 0.751, 1.502, 3.004},
	{0.300, 0.601, 1.202, 2.403},
	{0.250, 0.501, 1.001, 2.003},
	{0.200, 0.401, 0.801, 1.602},
	{0.150, 0.300, 0.601, 1.202}
};

double gain_res_z[8][4] = {
	{1.210, 2.420, 4.840, 9.680},
	{0.968, 1.936, 3.872, 7.744},
	{0.726, 1.452, 2.904, 5.808},
	{0.605, 1.210, 2.420, 4.840},
	{0.484, 0.968, 1.936, 3.872},
	{0.403, 0.807, 1.613, 3.227},
	{0.323, 0.645, 1.291, 2.581},
	{0.242, 0.484, 0.968, 1.936}
};




int main(int argc, char* args[])
{
	parse_args(argc, args, &gl_options);
	autoconnect_serial();

	mlx_write(0x77, MT_RESET);
	SDL_Delay(100);
	mlx_write(0x77, MT_EXIT_MODE);
	SDL_Delay(100);
	mlx_write(0x77, MT_RESET);
	SDL_Delay(100);


	//mlx_write_register(0x77, 0x2, 0b11111100000);	//set res. works
	uint8_t res = 0;
	uint8_t gain = 7 & 0x7;

	mlx_write_register(0x77, 0x0, (gain << 4));
	SDL_Delay(100);
	printf("Write Gain Complete\r\n");

	uint16_t resw = (res << 5) | (res << 7) | (res << 9);
	mlx_write_register(0x77, 0x2, resw);	//set res. works
	SDL_Delay(100);
	printf("Write Res Complete\r\n");

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
		if (gl_options.print_only == 0)
		{
			window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
			if (window == NULL)
			{
				printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			}
		}
		
		{
			SDL_Renderer* pRenderer = NULL;
			if(window != NULL)
			{
				pRenderer = SDL_CreateRenderer(window, -1, 0);
				SDL_SetRenderDrawColor(pRenderer, 255, 255, 255, 255);
			}
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
			int gl_selected_channel = 0;
			int number_of_frames_buffered = 0;
			uint64_t new_pkt_received_ts = 0;
			uint8_t timeout_occurred = 0;


			uint8_t write_override = 0;
			

			while (quit == false) 
			{
				uint64_t tick = SDL_GetTicks64() - start_tick;
				float t = ((float)tick) * 0.001f;
				if(pRenderer != NULL)
				{
					SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
					SDL_RenderClear(pRenderer);
				}

				uint8_t new_pkt = 0;
				if (gl_options.write_dummy_loopback)
				{
					write_dummy_loopback(SDL_GetTicks64(), &write_override);
				}

				pld_size = 0;
				int num_bytes_read = read_serial(gl_ser_readbuf, sizeof(gl_ser_readbuf));				
				for (int i = 0; i < (int)num_bytes_read; i++)
				{
					uint8_t new_byte = gl_ser_readbuf[i];
					pld_size = parse_PPP_stream(new_byte, gl_ppp_payload_buffer, PAYLOAD_SIZE, gl_ppp_unstuffing_buffer, UNSTUFFING_BUFFER_SIZE, &gl_ppp_bidx);
					if (pld_size > 0)
					{
						if (gl_options.offaxis_encoder)
						{
							parse_PPP_offaxis_encoder(gl_ppp_payload_buffer, pld_size, gl_valdump, &wordsize, SDL_GetTicks64());
							new_pkt = 1;
						}
						else if (gl_options.temp_sensor)
						{
							parse_PPP_tempsensor(gl_ppp_payload_buffer, pld_size, gl_valdump, &wordsize);
							new_pkt = 1;
						}
						else if (gl_options.fsr_sensor)
						{
							parse_PPP_fsr_sensor(gl_ppp_payload_buffer, pld_size, gl_valdump, &wordsize, SDL_GetTicks64());
							new_pkt = 1;
						}
						else
						{
							if (gl_options.xy_mode == 0)
							{
								parse_PPP_values(gl_ppp_payload_buffer, pld_size, gl_valdump, &wordsize);
								if (gl_options.mlx_sensor)
								{
									write_override = 1;
									gl_valdump[0] *= gain_res_xy[gain][res];
									gl_valdump[1] *= gain_res_xy[gain][res];
									gl_valdump[2] *= gain_res_z[gain][res];
								}
								new_pkt = 1;
							}
							else
							{
								parse_PPP_values_noscale(gl_ppp_payload_buffer, pld_size, gl_valdump, &wordsize);
								new_pkt = 1;
							}
						}

						//obtain consecutive matching counts
						if (wordsize == previous_wordsize && wordsize > 0)
						{
							wordsize_match_count++;
							if (gl_options.print_vals || gl_options.print_only)
							{
								cycle_count_for_printing = (cycle_count_for_printing + 1) % gl_options.print_in_parser_every_n;
								if (cycle_count_for_printing == 0)
								{
									for (int fvidx = 0; fvidx < wordsize; fvidx++)
									{
										float val = gl_valdump[fvidx] * gl_options.parser_yscale;
										printf("%0.6f", val);
										if (fvidx < (wordsize - 1))
											printf(", ");
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
					}
				}

				if (gl_options.print_only == 0)
				{
					//resize action
					if (wordsize_match_count >= 100)
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
							for (int line = 0; line < num_lines; line++)
							{
								for (int vidx = 0; vidx < dbufsize; vidx++)
								{
									fpoints_lines[line][vidx].x = 0;
									fpoints_lines[line][vidx].y = 0;
								}
							}
							number_of_frames_buffered = 0;
						}

					}
					{
						uint64_t time_since_last_new_packet = (tick - new_pkt_received_ts);
						if (time_since_last_new_packet > 400)
						{
							timeout_occurred = 1;
						}
						if (timeout_occurred != 0 && time_since_last_new_packet < 100)
						{
							timeout_occurred = 0;
							number_of_frames_buffered = 0;
						}
					}

					if(new_pkt)
					{
						number_of_frames_buffered++;	//maximum value is dbufsize
						if (number_of_frames_buffered >= dbufsize)
						{
							number_of_frames_buffered = dbufsize - 1;
						}
						new_pkt_received_ts = tick;
					}

					int last_idx = dbufsize - 1;	//most recent value
					int begin_idx = dbufsize - number_of_frames_buffered;
					//sanity bounds check. When the number of buffered frames is zero the above math would overrun
					if (begin_idx > last_idx)
						begin_idx = last_idx;
					if (begin_idx < 0)
						begin_idx = 0;

					for (int line = 0; line < fpoints_lines.size(); line++)
					{	
						if(pRenderer != NULL)
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

						if (line == 0)
						{
							if (gl_options.xy_mode == 0)
							{
								float div = (fpoints_lines[0][last_idx].x - fpoints_lines[0][begin_idx].x);
								if (div > 0)
									xscale = ((float)SCREEN_WIDTH) / div;
							}
						}

						for (int i = begin_idx; i < dbufsize; i++)
						{
							if (gl_options.spread_lines)
							{
								div_center = (div_pixel_size * line) + (div_pixel_size * .5f);	//calculate the center point of the line we're drawing on screen
							}
							if (gl_options.xy_mode == 0)
							{
								points[i].x = (int)(((*pFpoints)[i].x - (*pFpoints)[begin_idx].x) * xscale);
								points[i].y = SCREEN_HEIGHT - ((int)((*pFpoints)[i].y * gl_options.yscale) + div_center);
							}
							else
							{
								points[i].x = (int)(((*pFpoints)[i].x) * gl_options.yscale) + SCREEN_WIDTH /  2;
								points[i].y = (-(int)((*pFpoints)[i].y * gl_options.yscale)) + SCREEN_HEIGHT / 2;	//apply uniform scaling
							}
						}
						if (pRenderer != NULL)
							SDL_RenderDrawLines(pRenderer, (SDL_Point*)(&points[begin_idx]), dbufsize-begin_idx);
					}
					if (pRenderer != NULL)
						SDL_RenderPresent(pRenderer);				
				}
				SDL_PollEvent(&e);
				{
					if (e.type == SDL_QUIT)
						quit = true;
					else if (e.type == SDL_KEYDOWN)
					{
						int keyval = (int)e.key.keysym.sym;
					}
					else if (e.type == SDL_KEYUP)
					{
						int keyval = (int)e.key.keysym.sym;
						if (keyval == SDLK_UP)
						{
							//gl_selected_channel = (gl_selected_channel + 1) % NUM_FSR_PER_FINGER;
							gain++;
							gain &= 7;
							printf("gain = %d\r\n", gain);
							mlx_write_register(0x77, 0x0, (gain << 4));
							SDL_Delay(50);
						}
						if (keyval == SDLK_DOWN)
						{
							//gl_selected_channel = (gl_selected_channel - 1) % NUM_FSR_PER_FINGER;
							//if (gl_selected_channel < 0)
							//	gl_selected_channel = NUM_FSR_PER_FINGER + gl_selected_channel;
							gain--;
							gain &= 7;
							printf("gain = %d\r\n", gain);
							mlx_write_register(0x77, 0x0, (gain << 4));
							SDL_Delay(50);

						}
						if (keyval == SDLK_r)
						{
							res++;
							res &= 3;
							printf("res=%d\r\n", res);
							uint16_t resw = (res << 5) | (res << 7) | (res << 9);
							mlx_write_register(0x77, 0x2, resw);	//set res. works
							SDL_Delay(50);
						}
						if (keyval == SDLK_f)
						{
							res--;
							res &= 3;
							printf("res=%d\r\n", res);
							uint16_t resw = (res << 5) | (res << 7) | (res << 9);
							mlx_write_register(0x77, 0x2, resw);	//set res. works
							SDL_Delay(50);

						}

					}

				}

			}

			//erase all the buffers
			for (int i = 0; i < fpoints_lines.size(); i++)
			{
				fpoints_lines[i].clear();
			}
			fpoints_lines.clear();
			points.clear();
			if (pRenderer != NULL)
				SDL_DestroyRenderer(pRenderer);
		}
	}
	if(window != NULL)
	{
		//Destroy window
		SDL_DestroyWindow(window);
	}

	//Quit SDL subsystems
	SDL_Quit();

	close_serial();

	return 0;
}
