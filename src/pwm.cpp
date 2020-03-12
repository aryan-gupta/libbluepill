
#include <initializer_list>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include "pwm.hpp"

PWM::PWM(PWM_PIN pin, uint32_t init)
: timer{ pin_mapping[pin].timer }
, id{ pin_mapping[pin].id }
{
	rcc_periph_clock_enable(pin_mapping[pin].rcc_port);
	rcc_periph_clock_enable(pin_mapping[pin].rcc_timer);

	gpio_set_mode(
		pin_mapping[pin].port,
		GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		pin_mapping[pin].pin
	);

	timer_set_mode(
		timer,
		TIM_CR1_CKD_CK_INT,
		TIM_CR1_CMS_EDGE,
		TIM_CR1_DIR_UP
	);

	timer_set_prescaler(timer, 24);
	timer_set_period(timer, 60000); // this would give us a 50Hz frequency

	timer_enable_oc_output(timer, id);
	timer_set_oc_mode(timer, id, TIM_OCM_PWM1);
	timer_set_oc_value(timer, id, init);
	timer_enable_oc_preload(timer, id);

	timer_enable_counter(timer);
}

void PWM::update_pwm_value(uint16_t value) {
	timer_set_oc_value(timer, id, value);
}

uint16_t PWM::get_pwm_value() {
	switch (id) {
	case TIM_OC1:
		return TIM_CCR1(timer);
	case TIM_OC2:
		return TIM_CCR2(timer);
	case TIM_OC3:
		return TIM_CCR3(timer);
	case TIM_OC4:
		return TIM_CCR4(timer);
	case TIM_OC1N:
	case TIM_OC2N:
	case TIM_OC3N:
		/* Ignoring as this option applies to the whole channel. */
		return -1;
	}
}

void PWM::start() {
	timer_enable_oc_output(timer, id);
}

void PWM::stop() {
	timer_disable_oc_output(timer, id);
}