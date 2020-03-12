
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "solenoid.hpp"

bool Solenoid::SETUP = false;

Solenoid::Solenoid() {
	if (!SETUP) {
		gpio_solenoids_setup();
		SETUP = true;
	}
}

void Solenoid::gpio_solenoids_setup() {
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	gpio_set_mode(
		GPIOA,
		GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO11 | GPIO12 | GPIO15
	);

	gpio_set_mode(
		GPIOB,
		GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO3 | GPIO4 | GPIO5
	);
}

void Solenoid::set_high(mask_t mask) {
	gpio_toggle(GPIOB, GPIO3 | GPIO4 | GPIO5);
}

void Solenoid::set_low(mask_t mask) {
	gpio_toggle(GPIOB, GPIO3 | GPIO4 | GPIO5);
}