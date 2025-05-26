/*This source code copyrighted by Lazy Foo' Productions 2004-2023
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>

#include <math.h>
#include <vector>
#include "UdpSocket.h"
#include "PPP.h"
#include "ppp-parsing.h"
#include "colors.h"
#include "args-parsing.h"
#include <algorithm>
#include "trig_fixed.h"





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

	UdpSocket client;
	client.setNonBlocking();
	client.bindTo("0.0.0.0", 6701);  // Bind to all interfaces
	client.setTarget("192.168.123.86", 6701);
	client.sendTo("SPAM_ME", 7);

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

			SDL_Event e; 
			bool quit = false; 


			uint64_t start_tick = SDL_GetTicks64();

			SDL_SetRelativeMouseMode(SDL_TRUE);
			int prev_mouse_x = SCREEN_WIDTH / 2;
			int prev_mouse_y = SCREEN_HEIGHT / 2;
			int32_t accum_mouse_x = 0;
			int32_t accum_mouse_y = 0;
			double forward = 0;

			SDL_WarpMouseInWindow(window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

			bool keys[322];  // 322 is the number of SDLK_DOWN events

			for (int i = 0; i < 322; i++) { // init them all to false
				keys[i] = false;
			}

			int32_t delta = 0;
			uint64_t ms_ts = 0;

			uint64_t w_ts = 0;
			uint64_t s_ts = 0;
			bool w_pressed_prev = false;
			bool s_pressed_prev = false;

			while (quit == false) 
			{
				uint64_t tick = SDL_GetTicks64() - start_tick;
				if (tick - ms_ts > 0)
				{
					delta = (int32_t)(tick - ms_ts);
					ms_ts = tick;
				}
				float t = ((float)tick) * 0.001f;


				int mouse_x, mouse_y;
				SDL_GetMouseState(&mouse_x, &mouse_y);
				int32_t gain_x = 20;
				int32_t gain_y = 10;
				accum_mouse_x = ( (accum_mouse_x + (mouse_x - prev_mouse_x)* gain_x) );
				accum_mouse_y = ( ( accum_mouse_y + (mouse_y - prev_mouse_y)* gain_y) );
				accum_mouse_y = symm_thresh(accum_mouse_y, PI_14B / 2);
				int32_t w1 = (-accum_mouse_x + (int32_t)forward);
				int32_t w2 = (-accum_mouse_x - (int32_t)forward);
				int32_t gy = wrap_2pi_14b(accum_mouse_y);
				
				double accel_thresh = 10.0;
				if (keys[SDLK_w])
				{
					double dt = (double)(tick - w_ts) * 0.001;
					if (dt > accel_thresh)
						dt = accel_thresh;
					forward += (double)delta * dt;
				}
				if (keys[SDLK_s])
				{
					double dt = (double)(tick - s_ts) * 0.001;
					if (dt > accel_thresh)
						dt = accel_thresh;
					forward -= (double)delta * dt;
				}
				if (keys[SDLK_w] == true && w_pressed_prev == false)
				{
					w_ts = tick;
				}
				if (keys[SDLK_s] == true && s_pressed_prev == false)
				{
					s_ts = tick;
				}
				w_pressed_prev = keys[SDLK_w];
				s_pressed_prev = keys[SDLK_s];


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
				client.sendTo((const char*)gl_ppp_stuffing_buffer, stuffed_size);


				SDL_PollEvent(&e);
				{ 
					if (e.type == SDL_QUIT) 
						quit = true; 
					else if (e.type == SDL_KEYDOWN)
					{
						int idx = (int)e.key.keysym.sym;
						if (idx >= 0 && idx <= sizeof(keys) / sizeof(bool))
						{
							keys[idx] = true;
						}
					}
					else if (e.type == SDL_KEYUP)
					{
						int idx = (int)e.key.keysym.sym;
						if (idx >= 0 && idx <= sizeof(keys) / sizeof(bool))
						{
							keys[idx] = false;
						}
					}
				} 
			}
		}
	}

	//Destroy window
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();

	printf("Tore everything down, exiting nicely\n");
	return 0;
}
