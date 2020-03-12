
#pragma once

#include <array>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

class GPIO {
public:
	enum GPIO_PIN {
		A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15,
		B0, B1, B2, B3, B4, B5, B6, B7, B8, B9, B10, B11, B12, B13, B14, B15,
		C13, C14, C15,
		NUM_GPIO
	};
private:

	GPIO_PIN pin_idx;

	struct PORT {
		rcc_periph_clken port_clken;
		uint32_t port;
		uint16_t pin;
	};

	static constexpr std::array<PORT, NUM_GPIO> pin_mapping = []() constexpr {
		constexpr uint16_t gpio_pins_array[] = { GPIO0, GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7, GPIO8, GPIO9, GPIO10, GPIO11, GPIO12, GPIO13, GPIO14, GPIO15 };
		constexpr uint32_t gpio_ports_array[] = { GPIOA, GPIOB };
		constexpr rcc_periph_clken rcc_gpio_ports_array[] = { RCC_GPIOA, RCC_GPIOB };

		std::array<PORT, 35> pins{  };

		size_t pins_idx = 0;
		for (uint8_t ii = 0; ii < (sizeof(gpio_ports_array) / sizeof(gpio_ports_array[0])); ++ii) {
			for (uint8_t jj = 0; jj < (sizeof(gpio_pins_array) / sizeof(gpio_pins_array[0])); ++jj) {
				pins[pins_idx++] = PORT{ rcc_gpio_ports_array[ii], gpio_ports_array[ii], gpio_pins_array[jj] };
			}
		}

		pins[pins_idx++] = PORT{ RCC_GPIOC, GPIOC, GPIO13 };
		pins[pins_idx++] = PORT{ RCC_GPIOC, GPIOC, GPIO14 };
		pins[pins_idx++] = PORT{ RCC_GPIOC, GPIOC, GPIO15 };

		return pins;
	}();

public:
	GPIO(GPIO_PIN pin, uint8_t cfg = GPIO_CNF_OUTPUT_PUSHPULL);

	void set();
	void clear();

	bool get();

	void toggle();
};