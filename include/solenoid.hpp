#pragma once

#include <array>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

class Solenoid {
	using mask_t = uint8_t;

public:
	static constexpr mask_t CH1 = (1 << 1);
	static constexpr mask_t CH2 = (1 << 2);
	static constexpr mask_t CH3 = (1 << 3);
	static constexpr mask_t CH4 = (1 << 4);
	static constexpr mask_t CH5 = (1 << 5);
	static constexpr mask_t CH6 = (1 << 6);

	static constexpr mask_t NUM_DEVICES = 6;

private:
	struct PORT {
		uint32_t port;
		uint16_t pin;
	};

	static bool SETUP;
	static constexpr std::array<PORT, NUM_DEVICES> pin_mapping = {
		PORT{ GPIOA, GPIO11 },
		PORT{ GPIOA, GPIO12 },
		PORT{ GPIOA, GPIO13 },
		PORT{ GPIOB, GPIO3  },
		PORT{ GPIOB, GPIO4  },
		PORT{ GPIOB, GPIO5  }
	};

	static void gpio_solenoids_setup();

public:
	Solenoid();
	void set_high(mask_t mask);
	void set_low(mask_t mask);

};