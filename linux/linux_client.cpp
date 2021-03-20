#include <unistd.h>     // UNIX standard function definitions
#include <iostream>
#include <getopt.h>     // Miscellaneous symbolic constants and types.
#include "uart.h"
#include "linux_client.h"

#define BAUDRATE    115200
#define MAX_DI        4

#define MSG_GETSTATS  1
#define MSG_SETDO     2
#define MSG_TEST      3

struct st_msg_do_val {
	uint8_t do_num;
	uint8_t do_val;
};

struct st_msg_stats {
	uint8_t do_mask;
};

struct st_msg {
	uint8_t type;
	uint8_t length;
	uint8_t payload[DATA_LEN-2];
};

void LinuxClient::usage(FILE *output) const
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

int32_t LinuxClient::parse(int32_t argc, char *const argv[])
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
		int32_t aux_do = 0;
		switch (c) {
		case 'p':
			argument = optarg;
			if (*argument == '=' || *argument == ':') {
				argument++;
			}
			dev_port = argument;
			break;
		case 'a':
			argument = optarg;
			if (*argument == '=' || *argument == ':') {
				argument++;
			}
			aux_do = atoi(argument) - 1;
			if ((aux_do < 0) || (aux_do >= MAX_DI)) {
				std::cout << "Invalid Relay Number " << aux_do+1 << std::endl;
				break;
			}
			n_do = aux_do;
			act_do = true;
			break;
		case 'd':
			argument = optarg;
			if (*argument == '=' || *argument == ':') {
				argument++;
			}
			aux_do = atoi(argument) - 1;
			if ((aux_do < 0) || (aux_do >= MAX_DI)) {
				std::cout << "Invalid Relay Number " << aux_do+1 << std::endl;
				break;
			}
			n_do = aux_do;
			deact_do = true;
			break;
		case 's':
			get_stats = true;
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

int32_t LinuxClient::connect(void)
{
	if (serial.begin(dev_port.c_str(), BAUDRATE) < 0) {
		return -1;
	}
	return 0;
}

void LinuxClient::exec(void)
{
	UartComms UART_comms;
	UART_comms.begin(serial);

	if (act_do || deact_do) {
		struct st_msg *msg = (struct st_msg *)(&UART_comms.outgoingArray[0]);
		msg->type = MSG_SETDO;
		msg->length = sizeof(struct st_msg_do_val);

		struct st_msg_do_val *payload = (struct st_msg_do_val *)(&msg->payload[0]);

		payload->do_num = (uint8_t)n_do;
		payload->do_val = (uint8_t)(act_do && !deact_do);

		UART_comms.sendData(msg->length+2);

		#if DEBUG_MSG
			std::cout << "Send Data type: " << (int)msg->type << " length: " << (int)msg->length << std::endl;
			std::cout << "do_num " << (int)payload->do_num << " val: " << (int)payload->do_val << std::endl;
		#endif
	}

	#if TEST
		/* Request of TEST */
		{
			struct st_msg *msg = (struct st_msg *)(&UART_comms.outgoingArray[0]);
			msg->type = MSG_TEST;
			msg->length = 0;
			for (uint32_t i = 0; i < 10; i++) {
				msg->payload[msg->length++] = i+20;
			}
			UART_comms.sendData(msg->length+2);
			#if DEBUG_MSG
				std::cout << "Send Data type: " << (int)msg->type << " length: " << (int)msg->length << std::endl;
			#endif
		}
		/* Read Test */
		{
			while (true) {
				int32_t report = UART_comms.getData();

				if (report == 1) {
					struct st_msg msg;
					memcpy(&msg, &UART_comms.incomingArray[0], sizeof(struct st_msg));

					std::cout << "msg type: " << (uint32_t)msg.type << ", length: " << (uint32_t)msg.length << std::endl;
					for (uint32_t i = 0; i < msg.length; i++) {
						std::cout << i << " -- " << (int)msg.payload[i] << std::endl;
					}
					break;
				}
			}
		}
	#endif

	if (get_stats) {
		/* Request of statistics */
		{
			struct st_msg *msg = (struct st_msg *)(&UART_comms.outgoingArray[0]);
			msg->type = MSG_GETSTATS;
			msg->length = 0;

			UART_comms.sendData(msg->length+2);
			#if DEBUG_MSG
				std::cout << "Send Data type: " << (int)msg->type << " length: " << (int)msg->length << std::endl;
			#endif
		}

		/* Read answer */
		while (true) {
			int32_t report = UART_comms.getData();

			if (report == 1) {
				struct st_msg msg;
				memcpy(&msg, &UART_comms.incomingArray[0], sizeof(struct st_msg));

				#if DEBUG_MSG
					std::cout << "msg type: " << (uint32_t)msg.type << ", length: " << (uint32_t)msg.length << std::endl;
					for (uint32_t i = 0; i < msg.length; i++) {
						std::cout << i << " -- " << (int)msg.payload[i] << std::endl;
					}
				#endif

				if (msg.type != MSG_GETSTATS) {
					std::cerr << "Not expected msg (type: " << (uint32_t)msg.type << ")" << std::endl;
				} else {
					struct st_msg_stats *msg_stats = (struct st_msg_stats *)(&msg.payload[0]);
					for (uint8_t i = 0; i < MAX_DI; i++) {
						std::cout << "Relay " << (int)(i+1) << ": " << (int)((msg_stats->do_mask & (1 << i)) > 0) << std::endl;
					}
				}
				break;
			}
		}
	}
}
