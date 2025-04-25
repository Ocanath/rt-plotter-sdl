#include "stdio.h"
#include "args-parsing.h"
//linux headers for serial port
#include <string.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <unistd.h> // write(), read(), close()
#include <asm/ioctls.h>
#include <asm/termbits.h>
#include <sys/ioctl.h> // Used for TCGETS2, which is required for custom baud rates

int serial_port = -1;

int autoconnect_serial(void)
{
	int found_a_serial_port = 0;
	
	for(int i = 0; i < 256; i++)
	{
		char filename[32] = {0};	//some large enough empty buffer
		sprintf(filename, "/dev/ttyUSB%d", i);
		serial_port = open("/dev/ttyUSB0", O_RDWR);
		if (serial_port < 0) 
		{
			printf("Error %i from open %s: %s\n", errno, filename, strerror(errno));
		}
		else
		{	
			struct termios2 tty;
			ioctl(serial_port, TCGETS2, &tty);
			tty.c_cflag     &=  ~CSIZE;		// CSIZE is a mask for the number of bits per character
			tty.c_cflag |= CS8; 			// 8 bits per byte (most common)
			tty.c_cflag &= ~PARENB; 		// Clear parity bit, disabling parity (most common)
			tty.c_cflag     &=  ~CSTOPB;	// one stop bit
			tty.c_cflag &= ~CRTSCTS;		//hw flow control off
			tty.c_cflag     |=  CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
			
			tty.c_oflag     =   0;              // No remapping, no delays
			tty.c_oflag     &=  ~OPOST;         // Make raw
			tty.c_iflag &= ~(IXON | IXOFF | IXANY);
			tty.c_iflag 	&= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
			tty.c_lflag &= ~ICANON;
			tty.c_lflag &= ~ECHO; // Disable echo
			tty.c_lflag		&= ~ECHOE;     // Turn off echo erase (echo erase only relevant if canonical input is active)
			tty.c_lflag		&= ~ECHONL;    //
			tty.c_lflag		&= ~ISIG;      // Disables recognition of INTR (interrupt), QUIT and SUSP (suspend) characters


			//custom baud rate
			tty.c_cflag &= ~CBAUD;
			tty.c_cflag |= CBAUDEX;
			// tty.c_cflag |= BOTHER;
			tty.c_ispeed = gl_options.baud_rate;
			tty.c_ospeed = gl_options.baud_rate;

			//timeout=0
			tty.c_cc[VTIME] = 0;
			tty.c_cc[VMIN] = 0;

			ioctl(serial_port, TCSETS2, &tty);

			if (!(gl_options.csv_header == 1 && (gl_options.print_only == 1 || gl_options.print_in_parser == 1)))
			{
				printf("connected to %s successfully\n", filename);
			}
			return 1;
		}
	}
	if (gl_options.csv_header == 0)
		printf("Exiting due to no serial port found\n");
	return 0;
}

int serial_write(uint8_t* data, int size)
{
	return write(serial_port, data, size);
}

int read_serial(uint8_t * readbuf, int bufsize)
{
	return read(serial_port, readbuf, bufsize );
}

void close_serial(void)
{
	close(serial_port);
}
