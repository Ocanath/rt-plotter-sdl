#include "ppp-parsing.h"
#include <math.h>

#ifdef PLATFORM_WINDOWS
#include "winserial.h"
#elif defined(PLATFORM_LINUX)
#include "linux-serial.h"
#endif

#include "PPP.h"
#include "magsensor.h"

int gl_ppp_bidx = 0;
uint8_t gl_ppp_payload_buffer[PAYLOAD_SIZE] = { 0 };	//buffer
uint8_t gl_ppp_unstuffing_buffer[UNSTUFFING_BUFFER_SIZE] = { 0 };
uint8_t gl_ser_readbuf[PAYLOAD_SIZE] = { 0 };
float gl_valdump[PAYLOAD_SIZE / sizeof(float)] = { 0 };


int32_t get_vq_duty_from_float(float input)
{
	int32_t val = (int32_t)(input * 3546.f);
	if (val < -3546)
		val = -3546;
	if (val > 3546)
		val = 3546;	//sqrt(3)/2 * 4096, fixed point representation of vq duty for inverse park and inverse clarke (svm)
	return val;
}



uint64_t dummy_loopback_txts = 0;
void write_dummy_loopback(uint64_t tick, uint8_t * override)
{
	if (tick - dummy_loopback_txts > 15)// || *override != 0)
	{
		///////////////////
		float time = ((float)tick)/1000.f;
		float duty = sin(time*.6);





		/////////////////////leave alone
		int32_t vq_write = get_vq_duty_from_float(duty);
		uint8_t stuff_buf[sizeof(vq_write) * 2 + 2] = {};
		dummy_loopback_txts = tick;
		int num_bytes = PPP_stuff((uint8_t*)(&vq_write), sizeof(int32_t), stuff_buf, sizeof(stuff_buf));
		serial_write(stuff_buf, num_bytes);
	}
}


/*
Generic hex checksum calculation.
TODO: use this in the psyonic API
*/
uint32_t fletchers_checksum32(uint32_t* arr, int size)
{
	int32_t checksum = 0;
	int32_t fchk = 0;
	for (int i = 0; i < size; i++)
	{
		checksum += (int32_t)arr[i];
		fchk += checksum;
	}
	return fchk;
}


/*
* Inputs:
*	input_buf: raw unstuffed data buffer
* Outputs:
*	parsed_data: floats, parsed from input buffer
* Returns: number of parsed values
*/
void parse_PPP_values_noscale(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size)
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


/*
* Inputs:
*	input_buf: raw unstuffed data buffer
* Outputs:
*	parsed_data: floats, parsed from input buffer
* Returns: number of parsed values
*/
void parse_PPP_values(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size)
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



void parse_PPP_tempsensor(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size)
{
	uint32_t* pbu32 = (uint32_t*)(&input_buf[0]);
	int32_t* pbi32 = (int32_t*)(&input_buf[0]);
	int wordsize = payload_size / sizeof(uint32_t);
	int i = 0;

	float rawtemp = (float)pbi32[0];
	float mV = ((rawtemp * 3.3f) / 4096.f) * 1000.f;
	float Temp = (5.f / 44.f) * ((5.f * sqrt(9111265.f - 1760.f * mV)) - 13501.f);
	parsed_data[0] = Temp;
	for (i = 1; i < wordsize - 1; i++)
	{
		parsed_data[i] = 0;
	}
	parsed_data[wordsize - 1] = ((float)pbu32[i]) / 1000.f;
	*parsed_data_size = wordsize;
}
