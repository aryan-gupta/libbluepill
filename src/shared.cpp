
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#include <FreeRTOS.h>
#include <task.h>

#include "shared.hpp"

void sv_call_handler(void) {
	vPortSVCHandler();
}

void pend_sv_handler(void) {
	xPortPendSVHandler();
}

void sys_tick_handler(void) {
	xPortSysTickHandler();
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
	(void) pxTask;
	(void) pcTaskName;

	rcc_periph_clock_enable(RCC_GPIOC);

	gpio_set_mode(
		GPIOC,
		GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO13
	);

	while (true) {
		gpio_toggle(GPIOC, GPIO13);

		for(uint64_t ii = 0; ii < 1'000'000; ++ii) {
			__asm__("nop");
		}
	}
}
