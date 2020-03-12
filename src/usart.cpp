
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>

#include <libopencm3/stm32/timer.h>

#include <FreeRTOS.h>
#include <task.h>

#include "usart.hpp"

static USART* usart1 = nullptr;
static USART* usart2 = nullptr;
static USART* usart3 = nullptr;

USART::USART(uint32_t usart_)
: txq{ xQueueCreate(128, sizeof(char)) }
, rxq{ xQueueCreate(128, sizeof(char)) }
, usart{ usart_ }
{
	rcc_periph_clken gpio_clken;
	rcc_periph_clken usart_clken;
	uint8_t irqn;
	uint32_t output_port, input_port;
	uint16_t output_pin, input_pin;

	switch (usart) {
		case USART1:
			usart1 = this;

			gpio_clken = RCC_GPIOA;
			usart_clken = RCC_USART1;
			irqn = NVIC_USART1_IRQ;
			output_port = GPIO_BANK_USART1_TX;
			output_pin = GPIO_USART1_TX;
			input_port = GPIO_BANK_USART1_RX;
			input_pin = GPIO_USART1_RX;
		break;

		case USART2:
			usart2 = this;

			gpio_clken = RCC_GPIOA;
			usart_clken = RCC_USART2;
			irqn = NVIC_USART2_IRQ;
			output_port = GPIO_BANK_USART2_TX;
			output_pin = GPIO_USART2_TX;
			input_port = GPIO_BANK_USART2_RX;
			input_pin = GPIO_USART2_RX;
		break;

		case USART3:
			usart3 = this;

			gpio_clken = RCC_GPIOB;
			usart_clken = RCC_USART3;
			irqn = NVIC_USART3_IRQ;
			output_port = GPIO_BANK_USART3_TX;
			output_pin = GPIO_USART3_TX;
			input_port = GPIO_BANK_USART3_RX;
			input_pin = GPIO_USART2_RX;
		break;

		default:
			return;
	}

	rcc_periph_clock_enable(gpio_clken);
	rcc_periph_clock_enable(usart_clken);

	nvic_enable_irq(irqn);

	gpio_set_mode(
		output_port,
		GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		output_pin
	);

	gpio_set_mode(
		input_port,
		GPIO_MODE_INPUT,
		GPIO_CNF_INPUT_FLOAT,
		input_pin
	);

	usart_set_mode(usart, USART_MODE_TX_RX);
	usart_set_baudrate(usart, 9600);
	usart_set_databits(usart, 8);
	usart_set_stopbits(usart, USART_STOPBITS_1);
	usart_set_parity(usart, USART_PARITY_NONE);
	usart_set_flow_control(usart, USART_FLOWCONTROL_NONE);

	usart_enable_rx_interrupt(usart);

	usart_enable(usart);
}

void USART::isr() {
	/* Check if we were called because of RXNE. */
	if (((USART_CR1(usart) & USART_CR1_RXNEIE) != 0) &&
	    ((USART_SR(usart) & USART_SR_RXNE) != 0)) {
		char ch = usart_recv(usart);
		xQueueSendToBackFromISR(this->rxq, &ch, nullptr);
	}

	/* Check if we were called because of TXE. */
	if (((USART_CR1(usart) & USART_CR1_TXEIE) != 0) &&
	    ((USART_SR(usart) & USART_SR_TXE) != 0)) {
		char ch = '\0';

		BaseType_t r = xQueueReceiveFromISR(this->txq, &ch, nullptr);

		if (r == pdFALSE) {
			usart_disable_tx_interrupt(usart);
			return;
		}

		usart_send(usart, ch);
	}
}

void USART::send(char* buffer) {
	size_t ii = 0;

	if (buffer[ii] == '\0')
		return;

	// Fill the buffer with 4 elements before enabling the interrupt
	while (buffer[ii] != '\0' and ii < 4) {
		BaseType_t r = xQueueSendToBack(this->txq, buffer + ii, 0);
		if (r == pdTRUE) {
			++ii;
		} else {
			taskYIELD();
		}
	}

	usart_enable_tx_interrupt(usart);

	// Fill the buffer with the rest of the elements
	while (buffer[ii] != '\0') {
		BaseType_t r = xQueueSendToBack(this->txq, buffer + ii, 0);
		if (r == pdTRUE) {
			++ii;
		} else {
			taskYIELD();
		}
	}
}

void USART::send(char* buffer, size_t len) {
	if (len == 0)
		return;

	xQueueSendToBack(this->txq, buffer++, 0);
	usart_enable_tx_interrupt(usart);

	for (size_t ii = 1; ii < len; ++ii) {
		BaseType_t r = xQueueSendToBack(this->txq, (buffer + ii), 0);
		if (r == pdTRUE) {
			++buffer;
		}
	}
}

size_t USART::recv(char* buffer, size_t len) {
	for (size_t ii = 0; ii < len; ++ii) {
		BaseType_t r = xQueueReceive(this->rxq, (buffer + ii), 0);
		if (r == pdFALSE)
			return ii;
	}
	return len;
}

void usart1_isr() {
	if (usart1 != nullptr) {
		usart1->isr();
	}
}

void usart2_isr() {
	if (usart2 != nullptr) {
		usart2->isr();
	}
}

void usart3_isr() {
	if (usart3 != nullptr) {
		usart3->isr();
	}
}