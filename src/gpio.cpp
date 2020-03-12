
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "gpio.hpp"

GPIO::GPIO(GPIO_PIN pin, uint8_t cfg)
: pin_idx{ pin }
{
	rcc_periph_clock_enable(pin_mapping[pin_idx].port_clken);

	switch (pin) {
	case A13: case A14: case A15:
		rcc_periph_clock_enable(RCC_AFIO);
		gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, 0);
	default:
		break;
	}

	gpio_set_mode(
		pin_mapping[pin_idx].port,
		GPIO_MODE_OUTPUT_2_MHZ,
		cfg,
		pin_mapping[pin_idx].pin
	);
}

void GPIO::set() {
	gpio_set(pin_mapping[pin_idx].port, pin_mapping[pin_idx].pin);
}

void GPIO::clear() {
	gpio_clear(pin_mapping[pin_idx].port, pin_mapping[pin_idx].pin);
}

void GPIO::toggle() {
	gpio_toggle(pin_mapping[pin_idx].port, pin_mapping[pin_idx].pin);
}

bool GPIO::get() {
	uint16_t pin = pin_mapping[pin_idx].pin;
	return gpio_get(pin_mapping[pin_idx].port, pin) & pin;
}