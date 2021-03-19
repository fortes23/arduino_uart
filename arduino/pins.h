#define MAX_DI 4

static const uint8_t DO_relays[MAX_DI] = {
	#ifdef ARDUINO_AVR_NANO
		2, 3, 4, 5
	#elif ARDUINO_AVR_PRO
		6, 7, 8, 9
	#else
		#error Usopported board selection
	#endif
};

static Button DI_buttons[MAX_DI] = {
	#ifdef ARDUINO_AVR_NANO
		10, 11, 12, 9
	#elif ARDUINO_AVR_PRO
		2, 3, 4, 5
	#else
		#error Unsupported board selection
	#endif
};

#if EXTERNAL_LED
	static const uint8_t LEDs_state[MAX_DI] = {
		6,
		7,
		8,
		8
	};
#endif
