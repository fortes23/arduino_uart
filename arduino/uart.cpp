
#include "uart.h"

//initialize the UartComms class
void UartComms::begin(Stream &stream)
{
	_serial = &stream;
}

//change the UART buffer timeout (10ms by default)
void UartComms::setReceiveTimout(byte _timeout)
{
	timeout = _timeout;
}

//find 8-bit checksum of message
bool UartComms::calculateChecksum(uint8_t len, byte *buff)
{
	//reset checksum
	checksum = 0;

	//check if len is valid
	if (len < BUFF_LEN) {
		//compute checksum
		for (byte i = 0; i < len; i++) {
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
		byte j = 0;
		for (byte i = 0; i < data_len; i++) {
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
	_serial->write(START_BYTE);
	
	//send payload data_len in bytes
	_serial->write(buff_len);

	//send payload
	for (byte i = 0; i < (buff_len); i++) {
		_serial->write(inBuff[i]);
	}

	//send checksum
	_serial->write(checksum);

	//send END_BYTE
	_serial->write(END_BYTE);

	return true;
}

//update incomingArray with new data if available
int8_t UartComms::getData()
{
	uint32_t startTime = 0;
	uint32_t endTime = 0;

	byte payloadLen = 0;

	bool startFound = false;

	//see if any data is in the serial buffer
	if (_serial->available()) {
		startTime = millis();
		endTime = millis();

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
			endTime = millis();

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
			startTime = millis();
			endTime = millis();

			//wait for rest of dataframe to arrive with timeout
			while (_serial->available() < (payloadLen + 2)) {  //add 2 (1 for checksum and 1 for END_BYTE)
				//update timer
				endTime = millis();

				//test for timeout
				if ((endTime - startTime) >= timeout) {
					//oops, data didn't arrive on time - better get back to processing other things
					return TIMEOUT_ERROR;
				}
			}

			//stuff all payload bytes in the buffer for processing
			for (byte i = 0; i < payloadLen; i++) {
				inBuff[i] = _serial->read();
			}

			//update checksum before processing
			calculateChecksum(payloadLen, &inBuff[0]);

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

void UartComms::processData(byte payloadLen)
{
	//check if payloadLen is valid
	// if ((payloadLen <= (DATA_LEN * 3)) && (!(payloadLen % 3))) {
		for (byte i = 0; i < payloadLen; i = i + 2) {
			//sanity check for messageID
			if (inBuff[i] <= DATA_LEN) {
				//stuff the 16-bit data (data arrives bigendian)
				incomingArray[inBuff[i]] = (inBuff[i + 1]);
			}
		}
	// }

	return;
}