#include "fsrs.h"
#include "PPP.h"
#include <stdio.h>
#include <math.h>


#define PLD_SIZE	28	//bytes


typedef union 
{
	uint8_t u8[PLD_SIZE];
	int16_t i16[PLD_SIZE / sizeof(int16_t)];
}packed_sensor_data_t;
packed_sensor_data_t gl_tx_buf = { 0 };

/*
Generic hex checksum calculation.
TODO: use this in the psyonic API
*/
uint16_t get_checksum16(uint16_t* arr, int size)
{
	int16_t checksum = 0;
	for (int i = 0; i < size; i++)
		checksum += (int16_t)arr[i];
	return -checksum;
}

uint32_t parsecouter = 0;
const uint32_t num_samples_to_skip_plotting = 100;
void parse_PPP_offaxis_encoder(uint8_t * input_buf, int payload_size, float * parsed_data, int* parsed_data_size, uint64_t ms_tick)
{
	if (payload_size <= sizeof(packed_sensor_data_t))
	{
		packed_sensor_data_t* pData = (packed_sensor_data_t*)(&input_buf[0]);
		float rawtemp = pData->i16[6];
		float mV = ((rawtemp * 3.3f) / 4096.f) * 1000.f;
		float Temp = (5.f / 44.f) * ((5.f * sqrt(9111265.f - 1760.f * mV)) - 13501.f);
		
		uint16_t checksum = get_checksum16( (uint16_t*)pData->i16,  payload_size/sizeof(int16_t) - 1);
		if((uint16_t)pData->i16[payload_size/sizeof(int16_t)-1] == checksum)
		{
			parsed_data[0] = (float)pData->i16[0];
			parsed_data[1] = Temp;
			parsed_data[2] = ((float)ms_tick) / 1000.f;
			*parsed_data_size = 3;
		}
		//else, fall through and do nothing
		//printf("Temperature C = %f, Time = %f\n", parsed_data[0], parsed_data[1]);
	}
}

/*
 * Load packed 12 bit values located in an 8bit array into
 * an unpacked (zero padded) 16 bit array. FSR utility function
 */
void unpack_8bit_into_12bit(uint8_t* arr, uint16_t* vals, int valsize)
{
	for (int i = 0; i < valsize; i++)
		vals[i] = 0;	//clear the buffer before loading it with |=
	for (int bidx = valsize * 12 - 4; bidx >= 0; bidx -= 4)
	{
		int validx = bidx / 12;
		int arridx = bidx / 8;
		int shift_val = (bidx % 8);
		vals[validx] |= ((arr[arridx] >> shift_val) & 0x0F) << (bidx % 12);
	}
}

void parse_PPP_fsr_sensor(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size, uint64_t ms_tick)
{
	uint16_t vals[6] = { 0 };
	unpack_8bit_into_12bit(input_buf, vals, 6);
	//printf("Datans: ");
	int i = 0;
	for (i = 0; i < 6; i++)
	{
		parsed_data[i] = (float)(vals[i]);
		//printf("%f, ", parsed_data[i]);
	}
	//printf("\r\n");
	parsed_data[i++] = ((float)ms_tick) / 1000.f;
	*parsed_data_size = i;
}


// void offaxis_encoder_parser(HANDLE* pSer, uint8_t * serial_readbuf, uint8_t * ppp_unstuffing_buffer, int unstuffing_size, int * ppp_bidx)
// {
// 	int pld_size = 0;
// 	while (1)
// 	{
// 		LPDWORD num_bytes_read = 0;
// 		pld_size = 0;
// 		int rc = ReadFile(*pSer, serial_readbuf, 512, (LPDWORD)(&num_bytes_read), NULL);	//should be a DOUBLE BUFFER!
// 		for (int i = 0; i < (int)num_bytes_read; i++)
// 		{
// 			uint8_t new_byte = serial_readbuf[i];
// 			pld_size = parse_PPP_stream(new_byte, gl_tx_buf.u8, sizeof(packed_sensor_data_t), ppp_unstuffing_buffer, unstuffing_size, ppp_bidx);
// 			if (pld_size > 0)
// 			{
// 				//for (int i = 0; i < pld_size; i++)
// 				//{
// 				//	printf("%.2X", gl_tx_buf.u8[i]);
// 				//}
// 				//printf("\n");
// 				float rawtemp = gl_tx_buf.i16[6];
// 				float mV = ((rawtemp * 3.3f) / 4096.f)*1000.f;
// 				float Temp = (5.f / 44.f) * ((5.f * sqrt(9111265.f - 1760.f * mV)) - 13501.f);

// 				printf("Temperature C = %f\n", Temp);
// 			}
// 		}
// 	}
// }
