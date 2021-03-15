#include "button.h"

Button::Button(pin_t pin) : pin(pin) {
	// Enable the internal pull-up resistor on
	// the pin with the button/switch
	pinMode(pin, INPUT_PULLUP);
}

Button::State Button::getState() {
	State rstate;
	bool state = digitalRead(pin); // read the button state
	if (millis() - prevBounceTime > debounceTime) { // wait for state to stabilize
		// Concatenate the old state (debouncedState) and the new state (state).
		// If the old state is 1 and the new state is 0, the result will be binary 10.
		// The result is then cast to a State (State::Falling in this case).
		rstate = static_cast<State>((debouncedState << 1) | state);
		debouncedState = state;
	} else {
		rstate = static_cast<State>((debouncedState << 1) | debouncedState);
	}
	if (state != prevState) { // Button is pressed, released or bounces
		prevBounceTime = millis();
		prevState = state;
	}
	return rstate;
}