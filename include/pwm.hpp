
#pragma once

#include<array>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

class PWM {
public:
	enum PWM_PIN {
		A0, // TIM2 C1
		A1, // TIM2 C2
		A2, // TIM2 C3
		A3, // TIM2 C4

		A6, // TIM3 C1
		A7, // TIM3 C2
		B0, // TIM3 C3
		B1, // TIM3 C4

		B6, // TIM4 C1
		B7, // TIM4 C2
		B8, // TIM4 C3
		B9, // TIM4 C4

		NUM_PWM
	};

private:
	struct PORT {
		uint32_t port;
		uint32_t timer;
		uint16_t pin;
		tim_oc_id id;
		rcc_periph_clken rcc_port;
		rcc_periph_clken rcc_timer;
	};

	static constexpr std::array<PORT, NUM_PWM> pin_mapping = {
		PORT{ GPIO_BANK_TIM2_CH1_ETR, TIM2, GPIO_TIM2_CH1_ETR, TIM_OC1, RCC_GPIOA, RCC_TIM2 },
		PORT{ GPIO_BANK_TIM2_CH2, TIM2, GPIO_TIM2_CH2, TIM_OC2, RCC_GPIOA, RCC_TIM2 },
		PORT{ GPIO_BANK_TIM2_CH3, TIM2, GPIO_TIM2_CH3, TIM_OC3, RCC_GPIOA, RCC_TIM2 },
		PORT{ GPIO_BANK_TIM2_CH4, TIM2, GPIO_TIM2_CH4, TIM_OC4, RCC_GPIOA, RCC_TIM2 },

		PORT{ GPIO_BANK_TIM3_CH1, TIM3, GPIO_TIM3_CH1, TIM_OC1, RCC_GPIOA, RCC_TIM3 },
		PORT{ GPIO_BANK_TIM3_CH2, TIM3, GPIO_TIM3_CH2, TIM_OC2, RCC_GPIOA, RCC_TIM3 },
		PORT{ GPIO_BANK_TIM3_CH3, TIM3, GPIO_TIM3_CH3, TIM_OC3, RCC_GPIOB, RCC_TIM3 },
		PORT{ GPIO_BANK_TIM3_CH4, TIM3, GPIO_TIM3_CH4, TIM_OC4, RCC_GPIOB, RCC_TIM3 },

		PORT{ GPIO_BANK_TIM4_CH1, TIM4, GPIO_TIM4_CH1, TIM_OC1, RCC_GPIOB, RCC_TIM4 },
		PORT{ GPIO_BANK_TIM4_CH2, TIM4, GPIO_TIM4_CH2, TIM_OC2, RCC_GPIOB, RCC_TIM4 },
		PORT{ GPIO_BANK_TIM4_CH3, TIM4, GPIO_TIM4_CH3, TIM_OC3, RCC_GPIOB, RCC_TIM4 },
		PORT{ GPIO_BANK_TIM4_CH4, TIM4, GPIO_TIM4_CH4, TIM_OC4, RCC_GPIOB, RCC_TIM4 }
	};

	uint32_t timer;
	tim_oc_id id;

public:
	PWM(PWM_PIN pin, uint32_t init = 3500);
	void update_pwm_value(uint16_t value);
	uint16_t get_pwm_value();
	void stop();
	void start();
};