
#include "Arduino.h"

#ifndef UartComms_cpp
#define UartComms_cpp

#define DATA_LEN    40
#define BUFF_LEN    DATA_LEN * 2
#define START_BYTE  0x7E  //dataframe start byte
#define END_BYTE    0xEF  //dataframe end byte

//incoming serial data/parsing errors
#define NO_DATA              0
#define SERIAL_BUFF_ERROR   -1
#define END_BYTE_ERROR      -2
#define CHECKSUM_ERROR      -3
#define TIMEOUT_ERROR       -4
#define PAYLOAD_ERROR       -5

class UartComms
{
public:
	//data received
	uint8_t incomingArray[DATA_LEN] = { 0 };
	//data to send
	uint8_t outgoingArray[DATA_LEN] = { 0 };
	//initialize the UartComms class
	void begin(Stream& stream);
	//change the UART buffer timeout (10ms by default)
	void setReceiveTimout(uint8_t timeout);
	//send a selection of data from outgoingArray
	bool sendData(uint8_t data_len);
	//update incomingArray with new data if available
	int8_t getData();

private:
	//serial stream
	Stream* _serial;
	//data processing buffers
	uint8_t inBuff[BUFF_LEN] = { 0 };
	//receive timeout of 10ms by default
	uint16_t timeout = 1000;
	//checksum for determining validity of transfer
	uint8_t checksum = 0;
	//find 8 - bit checksum of message
	bool calculateChecksum(uint8_t len, uint8_t *buff);
	//process raw data and stuff into dataArray
	void processData(uint8_t payloadLen);
};

#endif
