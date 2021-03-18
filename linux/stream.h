#ifndef Stream_cpp
#define Stream_cpp

#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <termios.h>

class Stream
{
public:
	int32_t begin(const char *filename, uint32_t baudrate);
	// write info
	void write(uint8_t *buffer, uint32_t length);
	void write(uint8_t val);
	void write(char *str);
	// write several values
	// available
	uint32_t available(void);
	// read
	int32_t read(void);
	// flush
	int32_t flush(void);
private:
	// serial stream
	uint32_t _serial_fd;
	// options
	struct termios options;
};

#endif