#ifndef FSRS_H
#define FSRS_H
#include <stdint.h>
#include "winserial.h"

#define NUM_FSR_PER_FINGER 6

void offaxis_encoder_parser(HANDLE* pSer, uint8_t * serial_readbuf, uint8_t * ppp_unstuffing_buffer, int unstuffing_size, int * ppp_bidx);
void parse_PPP_offaxis_encoder(uint8_t * input_buf, int payload_size, float * parsed_data, int* parsed_data_size, uint64_t ms_tick);
void parse_PPP_fsr_sensor(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size, uint64_t ms_tick);

#endif