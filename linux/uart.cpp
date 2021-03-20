
#include "uart.h"
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <ctime>

//initialize the UartComms class
void UartComms::begin(Stream &stream)
{
	_serial = &stream;
}

//change the UART buffer timeout (10ms by default)
void UartComms::setReceiveTimout(uint8_t _timeout)
{
	timeout = _timeout;
}

//find 8-bit checksum of message
uint8_t UartComms::calculateChecksum(uint8_t len, uint8_t *buff)
{
	uint8_t crc = 0;
	for (uint8_t i = 0; i < len; i++) {
		uint8_t inbyte = buff[i];
		for (uint8_t j = 0; j < 8; j++) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) {
				crc ^= 0x8C;
			}
			inbyte >>= 1;
		}
	}
	return crc;
}

//send a selection of data from outgoingArray
bool UartComms::sendData(uint8_t data_len)
{
	// Length higher than expected
	if (data_len > DATA_LEN) {
		return false;
	}

	uint8_t buff_len = data_len * 2;
	uint8_t auxBuff[BUFF_LEN];
	// Update auxiliar buffer
	{
		uint8_t j = 0;
		for (uint8_t i = 0; i < data_len; i++) {
			auxBuff[j++] = i; // message ID
			auxBuff[j++] = outgoingArray[i];
		}
	}

	//update checksum before sending dataframe
	uint8_t checksum = calculateChecksum(buff_len, &auxBuff[0]);

	//send START_BYTE
	_serial->write(START_BYTE);

	//send payload data_len in bytes
	_serial->write(buff_len);

	//send payload
	_serial->write(&auxBuff[0], buff_len);

	//send checksum
	_serial->write(checksum);

	//send END_BYTE
	_serial->write(END_BYTE);

	return true;
}

//update incomingArray with new data if available
int8_t UartComms::getData()
{
	std::time_t startTime = 0;
	std::time_t endTime = 0;

	uint8_t payloadLen = 0;

	bool startFound = false;

	//see if any data is in the serial buffer
	if (_serial->available()) {
		startTime = std::time(0);
		endTime = std::time(0);

		//process only what bytes are currently in the buffer when looking for the START_BYTE
		while (_serial->available()) {
			//check if any bytes in the buffer are 
			if (_serial->read() == START_BYTE) {
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
		}

		//determine if the start of frame byte was found
		if (startFound) {
			//wait for the payload byte
			while (_serial->available() == 0);

			//read in the number of bytes in the payload of the packet
			payloadLen = _serial->read();

			//sanity check for the payload length (should be a multiple of 2 - 1 byte for ID, 1 for raw data)
			if ((payloadLen > (DATA_LEN * 2)) || (payloadLen % 2)) {
				//oops, bad payload length value
				return PAYLOAD_ERROR;
			}

			//prime the timeout timer
			startTime = std::time(0);
			endTime = std::time(0);

			//wait for rest of dataframe to arrive with timeout
			while (_serial->available() < (payloadLen + 2)) {  //add 2 (1 for checksum and 1 for END_BYTE)
				//update timer
				endTime = std::time(0);

				//test for timeout
				if ((endTime - startTime) >= timeout) {
					//oops, data didn't arrive on time - better get back to processing other things
					return TIMEOUT_ERROR;
				}
			}

			uint8_t auxBuff[BUFF_LEN];
			//stuff all payload bytes in the buffer for processing
			for (uint8_t i = 0; i < payloadLen; i++) {
				auxBuff[i] = _serial->read();
			}

			//update checksum before processing
			uint8_t checksum = calculateChecksum(payloadLen, &auxBuff[0]);

			//test received checksum
			if (_serial->read() != checksum) {
				//dang, checksums don't match - can't trust the data - get back to the main code
				return CHECKSUM_ERROR;
			}

			//test END_BYTE
			if (_serial->read() != END_BYTE) {
				//ugh, END_BYTE wasn't found in the right spot - can't trust the data - get back to the main code
				return END_BYTE_ERROR;
			}

			//process raw data and stuff into dataArray only if all data validity tests are passed
			processData(payloadLen, &auxBuff[0]);

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

void UartComms::processData(uint8_t payloadLen, uint8_t *buff)
{
	//check if payloadLen is valid
	for (uint8_t i = 0; i < payloadLen; i = i + 2) {
		//sanity check for messageID
		if (buff[i] <= DATA_LEN) {
			incomingArray[buff[i]] = (buff[i + 1]);
		}
	}
}
