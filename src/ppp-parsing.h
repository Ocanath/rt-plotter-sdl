#ifndef PPP_PARSING_H
#define PPP_PARSING_H
#include "stdint.h"

#define PAYLOAD_SIZE 512
#define UNSTUFFING_BUFFER_SIZE (PAYLOAD_SIZE * 2 + 2)

extern int gl_ppp_bidx;
extern uint8_t gl_ppp_payload_buffer[PAYLOAD_SIZE];	//buffer
extern uint8_t gl_ppp_unstuffing_buffer[UNSTUFFING_BUFFER_SIZE];
extern uint8_t gl_ser_readbuf[PAYLOAD_SIZE];
extern float gl_valdump[PAYLOAD_SIZE / sizeof(float)];


void parse_PPP_values_noscale(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size);
void parse_PPP_values(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size);
void parse_PPP_tempsensor(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size);
void write_dummy_loopback(uint64_t tick);


#endif // !PPP_PARSING_H
