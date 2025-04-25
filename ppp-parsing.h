#ifndef PPP_PARSING_H
#define PPP_PARSING_H
#include "stdint.h"

#define PAYLOAD_SIZE 512
#define UNSTUFFING_BUFFER_SIZE (PAYLOAD_SIZE * 2 + 2)



void parse_PPP_values_noscale(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size);
void parse_PPP_values(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size);
void parse_PPP_tempsensor(uint8_t* input_buf, int payload_size, float* parsed_data, int* parsed_data_size);
void write_dummy_loopback(uint64_t tick);
uint16_t fletchers_checksum16(uint16_t* arr, int size);


#endif // !PPP_PARSING_H
#pragma once
