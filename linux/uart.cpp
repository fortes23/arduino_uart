
#include "uart.h"
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <iostream>
#include <sys/ioctl.h>
#include <ctime>

//initialize the UartComms class
int32_t UartComms::begin(const char *filename)
{
	timeout = 10;
	checksum = 0;
	// Open port
	_serial_fd = open(filename, O_RDWR | O_NOCTTY | O_NONBLOCK);//| O_NDELAY | O_SYNC);
	if (_serial_fd == -1){
		std::cerr << "Device " << filename << " cannot be opened." << std::endl;
		return -1;
	}

	struct termios options;

	fcntl(_serial_fd, F_SETFL, FNDELAY);                    // Open the device in nonblocking mode

	// Set parameters
	tcgetattr(_serial_fd, &options);                        // Get the current options of the port
	bzero(&options, sizeof(options));               // Clear all the options

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

	//options.c_cflag &= ~HUPCL; // disable hang-up-on-close to avoid reset

	options.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
	options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
	options.c_oflag &= ~OPOST; // make raw

	// Configure the device: 8 bits, no parity, no control
	// options.c_cflag |= (CLOCAL | CREAD | CS8);
	// options.c_iflag |= (IGNPAR | IGNBRK);

	// Timer unused
	options.c_cc[VTIME]=0;
	options.c_cc[VMIN]=0;

	// Activate the settings
	tcsetattr(_serial_fd, TCSANOW, &options);

	if (tcsetattr(_serial_fd, TCSAFLUSH, &options) < 0) {
		std::cerr << "Flush error" << std::endl;
		return -1;
	}
		system("stty -F /dev/ttyUSB0 sane raw -echo 115200");

	// tcflush(_serial_fd,TCIOFLUSH);

	return 0;
}

//change the UART buffer timeout (10ms by default)
void UartComms::setReceiveTimout(uint8_t _timeout)
{
	timeout = _timeout;
}

//find 8-bit checksum of message
bool UartComms::calculateChecksum(uint8_t len, uint8_t *buff)
{
	//reset checksum
	checksum = 0;

	//check if len is valid
	if (len < BUFF_LEN) {
		//compute checksum
		for (uint8_t i = 0; i < len; i++) {
			checksum = checksum + buff[i];
		}
		checksum = (~checksum) + 1;
	} else {
		//couldn't update checksum
		return false;
	}

	//checksum updated
	return true;
}

//send a selection of data from outgoingArray
bool UartComms::sendData(uint8_t data_len)
{
	// Length higher than expected
	if (data_len > DATA_LEN) {
		return false;
	}

	uint8_t buff_len = data_len * 2;

	// Update auxiliar buffer
	{
		uint8_t j = 0;
		for (uint8_t i = 0; i < data_len; i++) {
			inBuff[j++] = i; // message ID
			inBuff[j++] = outgoingArray[i];
		}
	}

	//update checksum before sending dataframe
	if (!calculateChecksum(buff_len, &inBuff[0])) {
		//couldn't update checksum - return to main code
		return false;
	}

	//send START_BYTE
	uint8_t aux = START_BYTE;
	write(_serial_fd, &aux, 1);
	
	//send payload data_len in bytes
	write(_serial_fd, &buff_len, 1);

	//send payload
	write(_serial_fd, &inBuff[0], buff_len);

	//send checksum
	write(_serial_fd, &checksum, 1);

	//send END_BYTE
	aux = END_BYTE;
	write(_serial_fd, &aux, 1);

	return true;
}

//update incomingArray with new data if available
int8_t UartComms::getData()
{
	// {
	// 	uint32_t bytes_avail;
	// 	ioctl(_serial_fd, FIONREAD, &bytes_avail);
	// 	for (uint32_t i = 0; i < bytes_avail; i++) {
	// 		uint8_t read_val;
	// 		read(_serial_fd, &read_val, 1);
	// 		std::cout << (char)read_val << std::endl;
	// 	}
	// 	return 0;
	// }
	std::time_t startTime = 0;
	std::time_t endTime = 0;

	uint8_t payloadLen = 0;

	bool startFound = false;

	//see if any data is in the serial buffer
	uint32_t bytes_avail;
	ioctl(_serial_fd, FIONREAD, &bytes_avail);
	if (bytes_avail) {
		startTime = std::time(0);
		endTime = std::time(0);

		//process only what bytes are currently in the buffer when looking for the START_BYTE
		while (bytes_avail) {
			//check if any bytes in the buffer are
			uint8_t start_byte = 0;
			uint32_t read_bytes = read(_serial_fd, &start_byte, 1);
			if (start_byte == START_BYTE) {
				//start of a dataframe has been found - time to start looking for the data
				startFound = true;

				//break the "while available" loop
				break;
			}

			//update timer
			endTime = std::time(0);

			//test for timeout
			if ((endTime - startTime) >= timeout) {
				//oops, data didn't arrive on time - better get back to processing other things
				return TIMEOUT_ERROR;
			}
			ioctl(_serial_fd, FIONREAD, &bytes_avail);
		}

		//determine if the start of frame byte was found
		if (startFound) {
			bytes_avail = 0;
			//wait for the payload byte
			while (bytes_avail == 0) {
				ioctl(_serial_fd, FIONREAD, &bytes_avail);
			}

			//read in the number of bytes in the payload of the packet
			
			read(_serial_fd, &payloadLen, 1);

			//sanity check for the payload length (should be a multiple of 3 - 1 byte for ID, 2 for raw data)
			if ((payloadLen > (DATA_LEN * 2)) || (payloadLen % 2)) {
				//oops, bad payload length value
				return PAYLOAD_ERROR;
			}

			//prime the timeout timer
			startTime = std::time(0);
			endTime = std::time(0);

			//wait for rest of dataframe to arrive with timeout
			while (bytes_avail < (payloadLen + 2)) {  //add 2 (1 for checksum and 1 for END_BYTE)
				//update timer
				endTime = std::time(0);

				//test for timeout
				if ((endTime - startTime) >= timeout) {
					//oops, data didn't arrive on time - better get back to processing other things
					return TIMEOUT_ERROR;
				}
				ioctl(_serial_fd, FIONREAD, &bytes_avail);
			}

			//stuff all payload bytes in the buffer for processing
			read(_serial_fd, &inBuff[0], payloadLen);

			//update checksum before processing
			calculateChecksum(payloadLen, &inBuff[0]);

			//test received checksum
			uint8_t aux_msg = 0;
			read(_serial_fd, &aux_msg, 1);
			if (aux_msg != checksum) {
				//dang, checksums don't match - can't trust the data - get back to the main code
				return CHECKSUM_ERROR;
			}

			//test END_BYTE
			read(_serial_fd, &aux_msg, 1);
			if (aux_msg != END_BYTE) {
				//ugh, END_BYTE wasn't found in the right spot - can't trust the data - get back to the main code
				return END_BYTE_ERROR;
			}

			//process raw data and stuff into dataArray only if all data validity tests are passed
			processData(payloadLen);

			//nocie, everything checked out
			return 1;
		} else {
			//looks like we had garbage bytes in the serial buffer
			return SERIAL_BUFF_ERROR;
		}
	}

	//no bytes to process
	return NO_DATA;
}

void UartComms::processData(uint8_t payloadLen)
{
	//check if payloadLen is valid
	// if ((payloadLen <= (DATA_LEN * 3)) && (!(payloadLen % 3))) {
		for (uint8_t i = 0; i < payloadLen; i = i + 2) {
			//sanity check for messageID
			if (inBuff[i] <= DATA_LEN) {
				incomingArray[inBuff[i]] = (inBuff[i + 1]);
			}
		}
	// }

	return;
}