#include "uart.h"
#include "button.h"

#define BAUDRATE 115200
#define MAX_DI 4
#define MAX_SIZE_PAYLOAD 24

#define RISING_EDGE(signal, prev_state) ((prev_state << 1) | (signal & 1) & 3) == 1
#define FALLING_EDGE(signal, prev_state) ((prev_state << 1) | (signal & 1) & 3) == 2

static const uint8_t DO_relays[MAX_DI] = {
	2,
	3,
	4,
	5
};

static Button DI_buttons[MAX_DI] = {
	10,
	11,
	12,
	9
};

#if EXTERNAL_LED
	static const uint8_t LEDs_state[MAX_DI] = {
		6,
		7,
		8,
		8
	};
#endif

UartComms UART_comms;
// static uint8_t Buttons_mask;
static uint8_t DO_mask;

void setup()
{
	// Open serial communication
	Serial.begin(BAUDRATE);
	while (!Serial);
	UART_comms.begin(Serial);

	// Init GPIOs
	for (uint8_t i = 0; i < MAX_DI; i++) {
		pinMode(DO_relays[i], OUTPUT);
		#if EXTERNAL_LED
			pinMode(LEDs_state[i], OUTPUT);
		#endif
	}
}

#define MSG_GETSTATS  1
#define MSG_SETDO     2
#define MSG_TEST      3

#define HEADER_MSG    2

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

static void SendDOStats(void)
{
	struct st_msg_ib *msg = (struct st_msg_ib *)(&UART_comms.outgoingArray[0]);
	msg->type = MSG_GETSTATS;
	msg->length = sizeof(struct st_msg_send_stats);

	struct st_msg_send_stats *payload = (struct st_msg_send_stats *)(&msg->payload[0]);
	payload->do_mask = DO_mask;

	UART_comms.sendData(msg->length + HEADER_MSG);
}

static uint8_t Get_UART_DO(struct st_msg_ib *msg, uint8_t new_do_mask)
{
	struct st_msg_do_val *payload = (struct st_msg_do_val *)(&msg->payload[0]);

	if (payload->do_val) {
		new_do_mask |= (1 << payload->do_num);
	} else {
		new_do_mask &= ~(1 << payload->do_num);
	}

	return new_do_mask;
}

static void SendTestMsg(struct st_msg_ib *msg)
{
	memcpy(&UART_comms.outgoingArray[0], msg, msg->length + HEADER_MSG);
	UART_comms.sendData(msg->length + HEADER_MSG);
}

static uint8_t Get_UART_Data(uint8_t new_do_mask)
{
	// Get statistics or new DI value
	int8_t report = UART_comms.getData();

	//figure out if data was available - if so, determine if the transfer successful
	if (report == 1) {
		struct st_msg_ib msg;
		memcpy(&msg, &UART_comms.incomingArray[0], sizeof(struct st_msg_ib));

		switch (msg.type) {
			case MSG_GETSTATS:
				SendDOStats();
				break;
			case MSG_SETDO:
				return Get_UART_DO(&msg, new_do_mask);
				break;
			case MSG_TEST:
				SendTestMsg(&msg);
				break;
			default:
				break;
		}
	}
	return new_do_mask;
}

static uint8_t Get_Buttons(uint8_t new_do_mask)
{
	// Change state DI according to the rising edge of the buttons
	for (uint8_t i = 0; i < MAX_DI; i++) {
		if (DI_buttons[i].getState() == Button::Rising) {
			Serial.print("Button: ");
			Serial.println(i);
			if ((new_do_mask & (1 << i)) > 0) {
				new_do_mask &= ~(1 << i);
			} else {
				new_do_mask |= (1 << i);
			}
		}
	}

	return new_do_mask;
}

void loop() {
	uint8_t change_di_mask = 0;
	uint8_t new_do_val = DO_mask;

	new_do_val = Get_UART_Data(DO_mask);
	new_do_val = Get_Buttons(new_do_val);

	// Update DO
	for (uint8_t i = 0; i < MAX_DI; i++) {
		if ((new_do_val & (1 << i)) > 0) {
			digitalWrite(DO_relays[i], HIGH);
			#if EXTERNAL_LED
				digitalWrite(LEDs_state[i], HIGH);
			#endif
		} else {
			digitalWrite(DO_relays[i], LOW);
			#if EXTERNAL_LED
				digitalWrite(LEDs_state[i], LOW);
			#endif
		}
	}

	DO_mask = new_do_val;
}
