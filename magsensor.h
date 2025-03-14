#ifndef MAGSENSOR_H
#define MAGSENSOR_H
#include <stdint.h>

enum
{
	MT_START_BURST = 0xF0,
	MT_START_WKUP_ON_CHANGE_MODE = 0xF1,
	MT_START_SINGLE_MEASUREMENT_MODE = 0xF2,
	MT_READ_MEASUREMENT = 0xF3,
	MT_READ_REGISTER = 0xF4,
	MT_WRITE_REGISTER = 0xF5,
	MT_EXIT_MODE = 0xF6,
	MT_MEMORY_RECALL = 0xF7,	//do not implement
	MT_MEMORY_STORE = 0xF8,		//do not implement
	MT_RESET = 0xF9,

	MT_READ_XYZ = 0xA0,	//wrapper for the read call
	MT_ENABLE_AUTOREAD = 0xA1,
	MT_DISABLE_AUTOREAD = 0xA2,
	MT_DISABLE_SUBTRACTION = 0xB0,
	MT_ENABLE_SUBTRACTION = 0xB1
};

#define MAGSENSOR_RS485ADDRESS 0x77	//hardcoded

void mlx_write(uint16_t address, uint8_t mstgtype);
void mlx_write_register(uint16_t rs485addr, uint8_t regaddr, uint16_t regval);

#endif // !MAGSENSOR_H
