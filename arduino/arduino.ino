#include "uart.h"
#include "button.h"
#include "pins.h"

#define BAUDRATE 115200

static UartComms UART_comms;
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
		DO_mask = 0;
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

struct st_msg_stats {
	uint8_t do_mask;
};


struct st_msg {
	uint8_t type;
	uint8_t length;
	uint8_t payload[DATA_LEN-2];
};

static void SendDOStats(void)
{
	struct st_msg *msg = (struct st_msg *)(&UART_comms.outgoingArray[0]);
	msg->type = MSG_GETSTATS;
	msg->length = sizeof(struct st_msg_stats);

	struct st_msg_stats *payload = (struct st_msg_stats *)(&msg->payload[0]);
	payload->do_mask = DO_mask;

	UART_comms.sendData(msg->length + HEADER_MSG);
}

static uint8_t Get_UART_DO(struct st_msg *msg, uint8_t new_do_mask)
{
	struct st_msg_do_val *payload = (struct st_msg_do_val *)(&msg->payload[0]);

	if (payload->do_val) {
		new_do_mask |= (1 << payload->do_num);
	} else {
		new_do_mask &= ~(1 << payload->do_num);
	}

	return new_do_mask;
}

static void SendTestMsg(struct st_msg *msg)
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
		struct st_msg msg;
		memcpy(&msg, &UART_comms.incomingArray[0], sizeof(struct st_msg));

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
	uint8_t new_do_val = DO_mask;

	new_do_val = Get_UART_Data(DO_mask);
	new_do_val = Get_Buttons(new_do_val);

	// Update DO
	for (uint8_t i = 0; i < MAX_DI; i++) {
		bool status = (new_do_val & (1 << i)) > 0;
		digitalWrite(DO_relays[i], !status);
		#if EXTERNAL_LED
			digitalWrite(LEDs_state[i], !status);
		#endif
	}

	DO_mask = new_do_val;
}
