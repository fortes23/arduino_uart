#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <iostream>
#include <getopt.h>     // Miscellaneous symbolic constants and types.
#include "uart.h"
#include "stream.h"

#define BAUDRATE    115200

#define MSG_GETSTATS  1
#define MSG_SETDO     2
#define MSG_TEST      3

std::string DevicePort;
bool Get_Statistics = false;
bool Activate = false;
bool Deactivate = false;
uint32_t DoNum;


struct st_msg_do_val {
	uint8_t do_num;
	uint8_t do_val;
};

struct st_msg_send_stats {
	uint8_t do_mask;
};


struct st_msg_ib {
	uint8_t type;
	uint8_t length;
	uint8_t payload[DATA_LEN-2];
};

void usage(FILE *output)// const
{
	fprintf(output,
	        "\n"
	        "Usage: linux_uart [OPTIONS]\n"
	        "\n"
	        "  -p  --port=DevicePort        Device port\n"
	        "  -a  --activate               Activate DO [number]\n"
	        "  -d  --deactivate             Deactivate DO [number]\n"
	        "  -s  --stat=statistics        Get ports state\n"
	        "  -h  --help                   Show this help\n"
	        "\n"
	);
}

int32_t parse(int32_t argc, char *const argv[])
{
	if (argc == 1) {
		usage(stdout);
		return 1;
	}

	while (true) {
		const static struct option long_options[] = {
			{ "port",        required_argument, NULL, 'p' },
			{ "activate",    required_argument, NULL, 'a' },
			{ "deactivate",  required_argument, NULL, 'd' },
			{ "stat",        no_argument,       NULL, 's' },
			{ "help",        no_argument,       NULL, 'h' },
			{ 0,             0,                 NULL, 0   }
		};

		int optindex = -1;
		int c = getopt_long(argc, argv, 
		                    "p:a:d:sh",
		                    long_options, &optindex);

		if (c == -1) {
			break;
		}

		const char *argument;
		switch (c) {
		case 'p':
			argument = optarg;
			if (*argument == '=' || *argument == ':') {
				argument++;
			}
			DevicePort = argument;
			break;
		case 'a':
			argument = optarg;
			if (*argument == '=' || *argument == ':') {
				argument++;
			}
			Activate = true;
			DoNum = atoi(argument);
			break;
		case 'd':
			argument = optarg;
			if (*argument == '=' || *argument == ':') {
				argument++;
			}
			Deactivate = true;
			DoNum = atoi(argument);
			break;
		case 's':
			Get_Statistics = true;
			break;
		case 'h':
			usage(stdout);
			return 1;
		}
	}

	// Check that there are no more arguments
	if (optind != argc) {
		fprintf(stderr, "\nInvalid number of arguments\n");
		usage(stdout);
		return -1;
	}

	// Return success
	return 0;
}


int main(int argc, char** argv)
{

	if (parse(argc, argv) != 0) {
		return -1;
	}

	Stream serial;
	if (serial.begin(DevicePort.c_str(), BAUDRATE) < 0) {
		exit(0);
	}

	UartComms UART_comms;
	UART_comms.begin(serial);

	if (Activate || Deactivate) {
		struct st_msg_ib *msg = (struct st_msg_ib *)(&UART_comms.outgoingArray[0]);
		msg->type = MSG_SETDO;
		msg->length = sizeof(struct st_msg_do_val);

		struct st_msg_do_val *payload = (struct st_msg_do_val *)(&msg->payload[0]);

		payload->do_num = (uint8_t)DoNum;
		payload->do_val = (uint8_t)(Activate && !Deactivate);

		UART_comms.sendData(msg->length+2);
		std::cout << "Send Data type: " << (int)msg->type << " length: " << (int)msg->length << std::endl;
		std::cout << "do_num " << (int)payload->do_num << " val: " << (int)payload->do_val << std::endl;
	}


	#if TEST
		{
			struct st_msg_ib *msg = (struct st_msg_ib *)(&UART_comms.outgoingArray[0]);
			msg->type = MSG_TEST;
			msg->length = 0;
			for (uint32_t i = 0; i < 10; i++) {
				msg->payload[msg->length++] = i+20;
			}
			UART_comms.sendData(msg->length+2);
			std::cout << "Send Data type: " << (int)msg->type << " length: " << (int)msg->length << std::endl;
		}
	#endif

	if (Get_Statistics) {
		/* Request of statistics */
		{
			struct st_msg_ib *msg = (struct st_msg_ib *)(&UART_comms.outgoingArray[0]);
			msg->type = MSG_GETSTATS;
			msg->length = 0;

			UART_comms.sendData(msg->length+2);
			std::cout << "Send Data type: " << (int)msg->type << " length: " << (int)msg->length << std::endl;
		}

		/* Read answer */
		while (true) {
			int32_t report = UART_comms.getData();
				//figure out if data was available - if so, determine if the transfer successful
			if (report == 1) {
				struct st_msg_ib msg;
				memcpy(&msg, &UART_comms.incomingArray[0], sizeof(struct st_msg_ib));
				std::cout << "msg type: " << (uint32_t)msg.type << ", length: " << (uint32_t)msg.length << std::endl;

				for (uint32_t i = 0; i < msg.length; i++) {
					std::cout << i << " -- " << (int)msg.payload[i] << std::endl;
				}
				break;
			}
		}
	}
}
