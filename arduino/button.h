
#pragma once

#include <Arduino.h>

typedef uint8_t pin_t;

class Button {
	public:
		/**
		 * @brief   Construct a new Button object on the given pin.
		 *
		 * @param   pin
		 *          The digital pin to read from. 
		 *          The internal pull-up resistor will be enabled.
		 */
		Button(pin_t pin); // Constructor

	enum State {
		Pressed = 0b00,
		Released = 0b11,
		Falling = 0b10,
		Rising = 0b01
	};

	/**
	 * @brief   Get the state of the button.
	 *
	 *   Debounce time: ├────┤
	 * 
	 *   Raw input:
	 *     HIGH   ──────┐      ┌──────┐ ┌─┐      ┌─┐ ┌──────┐ ┌────────
	 *     LOW          └──────┘      └─┘ └──────┘ └─┘      └─┘
	 *                  ├────┤ ├────┤ ├─┼─┼────┤ ├─┼─┼────┤ ├─┼────┤
	 *   Debounced output:
	 *     HIGH   ──────┐      ┌──────┐          ┌──────────┐      ┌───
	 *     LOW          └──────┘      └──────────┘          └──────┘
	 * 
	 *   States:
	 *     HIGH   ────────────────┐                  ┌─────────────────
	 *     LOW                    └──────────────────┘
	 *              Released   Falling   Pressed   Rising
	 *       
	 * @return  State
	 *          The state of the button, either Button::Pressed,
	 *          Button::Released, Button::Falling or Button::Rising.
	 *          The state is debounced.
	 */
	State getState(); // Check if the button state changed

	private:
		const pin_t pin;

		bool prevState = HIGH;
		bool debouncedState = HIGH;
		unsigned long prevBounceTime = 0;

		constexpr static unsigned long debounceTime = 100; // 100 milliseconds
};