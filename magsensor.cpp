#include "PPP.h"
#ifdef PLATFORM_WINDOWS
#include "winserial.h"
#elif defined(PLATFORM_LINUX)
#include "linux-serial.h"
#endif
#include "magsensor.h"



uint8_t pld[32] = {};
uint8_t stuff_pld[sizeof(pld) * 2 + 2] = {};


void mlx_write(uint16_t address, uint8_t mstgtype)
{
	uint16_t* pbu16 = (uint16_t*)(&pld[0]);
	pld[0] = address;
	pld[2] = mstgtype;

	int num_bytes = PPP_stuff(pld, sizeof(pld), stuff_pld, sizeof(stuff_pld));
	serial_write(stuff_pld, num_bytes);
}

void mlx_write_register(uint16_t rs485addr, uint8_t regaddr, uint16_t regval)
{
	uint16_t* pbu16 = (uint16_t*)(&pld[0]);
	pld[0] = rs485addr;
	pld[2] = MT_WRITE_REGISTER;
	pld[3] = regaddr;
	uint16_t* pregv = (uint16_t*)(&pld[4]);
	pregv[0] = regval;


	int num_bytes = PPP_stuff(pld, sizeof(pld), stuff_pld, sizeof(stuff_pld));
	serial_write(stuff_pld, num_bytes);

}
