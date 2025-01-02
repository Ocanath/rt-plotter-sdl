#ifndef LINUX_SERIAL_H
#define LINUX_SERIAL_H
#include <stdint.h>


int autoconnect_serial(void);
int serial_write(uint8_t* data, int size);
int read_serial(uint8_t* readbuf);
void close_serial(void);


#endif // ! LINUX_SERIAL_H
