#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <sys/ioctl.h>
#include <iostream>
#include "stream.h"

//initialize the UartComms class
int32_t Stream::begin(const char *filename, uint32_t baudrate)
{
	// Open port
	_serial_fd = open(filename, O_RDWR | O_NOCTTY | O_NONBLOCK);//| O_NDELAY | O_SYNC);
	if (_serial_fd == -1){
		std::cerr << "Device " << filename << " cannot be opened." << std::endl;
		return -1;
	}

	fcntl(_serial_fd, F_SETFL, FNDELAY); // Open the device in nonblocking mode

	// Set parameters
	tcgetattr(_serial_fd, &options);     // Get the current options of the port
	bzero(&options, sizeof(options));    // Clear all the options

	// Set the baudrate speed
	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	// 8N1
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	// no flow control
	options.c_cflag &= ~CRTSCTS;

	options.c_cflag |= CREAD | CLOCAL;          // turn on READ & ignore ctrl lines
	options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
	options.c_oflag &= ~OPOST; // make raw

	// Timer unused
	options.c_cc[VTIME]=0;
	options.c_cc[VMIN]=0;

	// Activate the settings
	tcsetattr(_serial_fd, TCSANOW, &options);

	if (flush() < 0) {
		return -1;
	}

	return 0;
}

void Stream::write(uint8_t *buffer, uint32_t length)
{
	::write(_serial_fd, buffer, length);
}

void Stream::write(uint8_t val)
{
	write(&val, sizeof(uint8_t));
}

void Stream::write(char *str)
{
	write((uint8_t *)str, strlen(str));
}

int32_t Stream::read(void)
{
	uint8_t value;

	if (::read(_serial_fd, &value, 1) < 0) {
		return -1;
	}

	return (int32_t)value;
}

uint32_t Stream::available(void)
{
	uint32_t bytes_avail;
	ioctl(_serial_fd, FIONREAD, &bytes_avail);

	return bytes_avail;
}

int32_t Stream::flush(void)
{
	if (tcsetattr(_serial_fd, TCSAFLUSH, &options) < 0) {
		std::cerr << "Flush error" << std::endl;
		return -1;
	}
	return 0;
}
