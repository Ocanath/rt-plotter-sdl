#include <SDL.h>
#include <stdio.h>
#include <vector>

// #include<winsock2.h>
// #include <WS2tcpip.h>
// #include "WinUdpClient.h"

#include <math.h>

#ifdef PLATFORM_WINDOWS
#include "winserial.h"
#elif defined(PLATFORM_LINUX)
#include "linux-serial.h"
#endif

#include "PPP.h"
#include "colors.h"
#include "args-parsing.h"
#include <algorithm>
#include "trig_fixed.h"
#include "sauron-eye-closedform-ik.h"
#include "ppp-parsing.h"
// #pragma comment(lib,"ws2_32.lib") //Winsock Library
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
	autoconnect_serial();//grab a serial port

	// WinUdpClient client(6702);
	// struct sockaddr_in server;
	// server.sin_family = AF_INET;
	// server.sin_addr = in4addr_any;
	// server.sin_port = htons(6702);
	// client.set_nonblocking();
	// client.si_other.sin_family = AF_INET;
	// //client.si_other.sin_addr.S_un.S_addr = inet_addr("192.168.123.255");
	// //TODO: auto address discovery with WHO_GOES_THERE
	// inet_pton(AF_INET, "192.168.137.145", &client.si_other.sin_addr);
	// sendto(client.s, (const char*)"SPAM_ME", 7, 0, (struct sockaddr*)&client.si_other, client.slen);
	// bind(client.s, (struct sockaddr*)&server, sizeof(server));



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
			const double slow_speedcap = 200;
			const double fast_speedcap = 3000;
			double velocity_threshold = slow_speedcap;
			double th1 = 0;
			double th2 = 0;

			SDL_WarpMouseInWindow(window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

			const int num_keys = 322;
			bool keys[num_keys];  // 322 is the number of SDLK_DOWN events

			for (int i = 0; i < num_keys; i++) { // init them all to false
				keys[i] = false;
			}

			int32_t delta = 0;
			uint64_t ms_ts = 0;

			uint64_t w_ts = 0;
			uint64_t s_ts = 0;
			uint64_t w_stop_ts = 0;
			uint64_t s_stop_ts = 0;
			bool w_pressed_prev = false;
			bool s_pressed_prev = false;
			int zerocount = 0;
			uint64_t mouse_activity_ts = 0;
			uint64_t print_ts = 0;
			uint64_t udp_tx_ts = 0;
			int32_t deltax_lastvalid = 0;
			uint8_t throttle = 0;
			uint8_t prev_throttle = 0;
			uint64_t throttle_stop_ts = 0;
			double targx = 0;
			double targy = 0;
			while (quit == false) 
			{
				uint64_t tick = SDL_GetTicks64() - start_tick;
				if (tick - ms_ts > 0)
				{
					delta = (int32_t)(tick - ms_ts);
					ms_ts = tick;
				}
				float t = ((float)tick) * 0.001f;

				uint8_t mouse_motion = 0;
				SDL_PollEvent(&e);
				{
					if (e.type == SDL_QUIT)
						quit = true;
					else if (e.type == SDL_MOUSEMOTION)
					{
						mouse_motion = 1;
						mouse_activity_ts = tick;
					}
				}

				int mouse_x, mouse_y;
				uint32_t bts = SDL_GetMouseState(&mouse_x, &mouse_y);
				int32_t gain_x = 1;
				int32_t gain_y = -1;
				int32_t deltax = (mouse_x - prev_mouse_x)* gain_x;
				int32_t deltay = (mouse_y - prev_mouse_y) * gain_y;

				accum_mouse_x = ( (accum_mouse_x + deltax) );
				accum_mouse_y = ( ( accum_mouse_y + deltay) );
				
				if (mouse_motion != 0 || (tick - mouse_activity_ts) > 10)
				{
					deltax_lastvalid = deltax;
				}



				double kbvx = -(double)accum_mouse_x / 1000.;
				double kbvy = (double)accum_mouse_y / 1000.;

				double vx = kbvx;
				double vy = kbvy;
				if (bts == 1)	//left mouse button
				{
					vx = targx;
					vy = targy;
				}
				if (bts == 2)	//center mouse button
				{
					targx = vx;
					targy = vy;
				}

				//double tsec = double(tick) / 1000;
				//double vx = sin(tsec)*0.5;
				//double vy = cos(tsec)*0.5;

				//double vx = 0;
				//double vy = 0;
				//uint32_t tmod = tick % 4000;
				//
				//if (tmod >= 0 && tmod < 1000)
				//{
				//	double tmodsec = (double)(tmod - 0) / 1000.;
				//	vx = tmodsec*10;
				//	vy = 0;
				//}
				//else if (tmod >= 1000 && tmod < 2000)
				//{
				//	double tmodsec = (double)(tmod - 1000) / 1000.;
				//	vx = 10;
				//	vy = tmodsec * 10;
				//}
				//else if (tmod >= 2000 && tmod < 3000)
				//{
				//	double tmodsec = (double)(tmod - 2000) / 1000.;
				//	vx = 10 - tmodsec * 10;
				//	vy = 10;
				//}
				//else if (tmod >= 3000 && tmod < 4000)
				//{
				//	double tmodsec = (double)(tmod - 3000) / 1000.;
				//	vy = 10 - tmodsec * 10;
				//	vx = 0;
				//}
				//vx = vx + kbvx;
				//vy = vy + kbvy;

				const double rotangle = M_PI / 4;
				double vxr = vx * cos(rotangle) - vy * sin(rotangle);
				double vyr = vx * sin(rotangle) + vy * cos(rotangle);
				get_ik_angles_double(vxr, vyr, 10, &th1, &th2);
				int32_t th1_i32 = (int32_t)(th1 * (float)PI_14B);
				int32_t th2_i32 = (int32_t)(th2 * (float)PI_14B);

				if ( (tick - print_ts) > 50)
				{
					printf("%f, %f, %d, %d, %d\n", vx, vy, th1_i32, th2_i32, bts);
					print_ts = tick;
				}

				SDL_WarpMouseInWindow(window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);


				if (tick - udp_tx_ts > 10)
				{
					udp_tx_ts = tick;

					uint8_t pld[32] = { 0 };
					uint16_t* pu16 = (uint16_t*)(&pld[0]);
					int i = 0;
					pld[i++] = POSITION;
					pld[i++] = 0;

					int32_t* p_cmd32 = (int32_t*)(&pld[i]);
					p_cmd32[0] = th1_i32;
					i += sizeof(int32_t);
					p_cmd32[1] = th2_i32;
					i += sizeof(int32_t);

					int chkidx = (i + (i % 2)) / 2;	//pad a +1 byte if it's odd, divide by 2, set that as the start of our 16bit checksum
					pu16[chkidx] = fletchers_checksum16(pu16, chkidx);
					chkidx++;
					int pld_size = chkidx * sizeof(uint16_t);
					int stuffed_size = PPP_stuff(pld, pld_size, gl_ppp_stuffing_buffer, sizeof(gl_ppp_stuffing_buffer));
					//sendto(client.s, (const char*)gl_ppp_stuffing_buffer, stuffed_size, 0, (struct sockaddr*)&client.si_other, client.slen);
					serial_write(gl_ppp_stuffing_buffer, stuffed_size);
				}
			}
		}
	}

	//Destroy window
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();

	// closesocket(client.s);
	// WSACleanup();
	close_serial();
	printf("Tore everything down, exiting nicely\n");
	return 0;
}
