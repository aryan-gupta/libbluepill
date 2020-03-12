
#include <stdint.h>
#include <string.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/can.h>

#include <libopencm3/stm32/timer.h>

#include "can.hpp"

static CAN* can_bus;

CAN::CAN(bool altcfg)
: txq{ xQueueCreate(16, sizeof(tx_frame)) }
, rxq{ xQueueCreate(128, sizeof(rx_frame)) }
{
	can_bus = this;

	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_CAN);

	rcc_periph_clken gpio_clken;
	uint32_t output_port, input_port;
	uint16_t output_pin, input_pin;
	uint32_t remap;

	if (altcfg) { // CAN_RX=PB8, CAN_TX=PB9
		gpio_clken = RCC_GPIOB;
		output_port = GPIO_BANK_CAN_PB_TX;
		output_pin = GPIO_CAN_PB_TX;
		input_port = GPIO_BANK_CAN_PB_RX;
		input_pin = GPIO_CAN_PB_RX;
		remap = AFIO_MAPR_CAN1_REMAP_PORTB;
	} else { // CAN_RX=PA11, CAN_TX=PA12
		gpio_clken = RCC_GPIOA;
		output_port = GPIO_BANK_CAN_TX;
		output_pin = GPIO_CAN_TX;
		input_port = GPIO_BANK_CAN_RX;
		input_pin = GPIO_CAN_RX;
		remap = AFIO_MAPR_CAN1_REMAP_PORTA;
	}

	rcc_periph_clock_enable(gpio_clken);
	gpio_set_mode(
		output_port,
		GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
		output_pin
	);

	gpio_set_mode(
		input_port,
		GPIO_MODE_INPUT,
		GPIO_CNF_INPUT_FLOAT,
		input_pin
	);
	// gpio_set(input_port, input_pin); // is thie really needed?

	gpio_primary_remap(
		AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON,
		remap
	);

	can_reset(CAN1);

	int r = can_init(
		CAN1,
		false,           /* TTCM: Time triggered comm mode? */
		false,           /* ABOM: Automatic bus-off management? */
		false,           /* AWUM: Automatic wakeup mode? */
		false,           /* NART: No automatic retransmission? */
		false,           /* RFLM: Receive FIFO locked mode? */
		false,           /* TXFP: Transmit FIFO priority? */
		CAN_BTR_SJW_1TQ,
		CAN_BTR_TS1_6TQ,
		CAN_BTR_TS2_7TQ,
		78,
		false,
		false /* BRP+1: Baud rate prescaler */
	);

	if (r) {
		/* Die because we failed to initialize. */
		while (1)
			__asm__("nop");
	}

	can_filter_id_mask_32bit_init(
		0,     /* Filter ID */
		0,     /* CAN ID */
		0,     /* CAN ID mask */
		0,     /* FIFO assignment (here: FIFO0) */
		true   /* Enable the filter. */
	);

	nvic_enable_irq(NVIC_CAN_RX1_IRQ);
	nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
	nvic_enable_irq(NVIC_USB_HP_CAN_TX_IRQ);

	can_enable_irq(CAN1, CAN_IER_FMPIE0 | CAN_IER_FMPIE1);
}

void CAN::rx_isr(uint8_t fifo, uint8_t num_msg) {
	rx_frame cmsg;
	cmsg.fifo = fifo;

	// recive all the messages and add them to the buffer
	while (num_msg --> 0) {
		can_receive(
			CAN1,
			cmsg.fifo,
			true,
			&cmsg.id,
			&cmsg.xid,
			&cmsg.rtr,
			&cmsg.fmi,
			&cmsg.len,
			cmsg.data,
			NULL
		);

		xQueueSendToBackFromISR(rxq, &cmsg, NULL);
	}
}

void CAN::tx_isr() {
	tx_frame cmsg;

	// keep adding messages until mailboxes are full or
	// until we run out of messages
	while (can_available_mailbox(CAN1)) {
		BaseType_t r = xQueueReceiveFromISR(this->txq, &cmsg, nullptr);

		// if we run out of messages then turn off the interrupt
		// as it is not needed anymore
		if (r == pdFALSE) {
			can_disable_irq(CAN1, CAN_IER_TMEIE);
			return;
		}

		can_transmit(
			CAN1,
			cmsg.id,
			cmsg.xid,
			cmsg.rtr,
			cmsg.len,
			cmsg.data
		);
	}
}


void CAN::send(tx_frame* buffer, size_t len) {
	// The CAN_IER_TMEIE interrupt is called ONLY when a mailbox becomes empty
	// if all the mailboxes are empty (e.x. when the uC resets), there wont be
	// an interrupt, this means we need to fill all the mailboxes before we can
	// enable the interrupt

	if (len == 0)
		return;

	size_t index = 0;

	// Populate all the mailboxes first before enableing the IRQ
	while (true) {
		int mailbox = can_transmit(
			CAN1,
			buffer[index].id,
			buffer[index].xid,
			buffer[index].rtr,
			buffer[index].len,
			buffer[index].data
		);

		if (mailbox == -1) { // mail boxes are full
			break; // break and queue them up
		}else {
			index++; // otherwise continue placing them in the mailbox
		}

		if (index == len) { // if we queued up all of the mail
			// then quit
			return;
		}
	}

	// if there is more mail after all the mailboxes are full
	// then queue them up and enable the IRQ
	xQueueSendToBack(this->txq, (buffer + index), 0);
	can_enable_irq(CAN1, CAN_IER_TMEIE);

	// queue up the rest of the mail
	while (index != len) {
		BaseType_t r = xQueueSendToBack(this->txq, (buffer + index), 0);
		if (r == pdTRUE) {
			++index;
		}
	}
}

size_t CAN::recv(rx_frame* buffer, size_t len) {
	for (size_t ii = 0; ii < len; ++ii) {
		BaseType_t r = xQueueReceive(this->rxq, (buffer + ii), 0);
		if (r == pdFALSE)
			return ii;
	}
	return len;
}

void usb_lp_can_rx0_isr(void) {
	can_bus->rx_isr(0, CAN_RF0R(CAN1) & 0x3);
}

void can_rx1_isr(void) {
	can_bus->rx_isr(1, CAN_RF1R(CAN1) & 0x3);
}

void usb_hp_can_tx_isr(void) {
	can_bus->tx_isr();
}
