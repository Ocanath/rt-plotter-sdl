#include "PPP.h"
#ifdef PLATFORM_WINDOWS
#include "winserial.h"
#elif defined(PLATFORM_LINUX)
#include "linux-serial.h"
#endif

uint8_t pld[32] = {};
uint8_t stuff_pld[sizeof(pld) * 2 + 2] = {};


void mlx_write(uint16_t address, uint8_t mstgtype)
{
	uint16_t* pbu16 = (uint16_t*)(&pld[0]);
	pld[2] = 0xA1;

	int num_bytes = PPP_stuff(pld, sizeof(pld), stuff_pld, sizeof(stuff_pld));
	serial_write(stuff_pld, num_bytes);
}