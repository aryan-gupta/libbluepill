
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/spi.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "spi.hpp"

static SPI* spi1 = nullptr;
static SPI* spi2 = nullptr;

SPI::SPI(uint32_t spi_, SPI_TYPE type_)
: txq{ xQueueCreate(128, sizeof(uint16_t)) }
, rxq{ xQueueCreate(128, sizeof(uint16_t)) }
, type{ type_ }
, spi{ spi_ }
{
	spi2 = this;

	rcc_periph_clken gpio_clken, spi_clken;
	uint8_t irqen;
	uint16_t output, input, nss;
	decltype(spi_init_master)* spi_init;

	switch (spi) {
		case SPI1: {
			gpio_clken = RCC_GPIOA;
			spi_clken = RCC_SPI1;
			irqen = NVIC_SPI1_IRQ;
			if (type == MASTER) {
				nss = GPIO_SPI1_NSS;
				output = nss | GPIO_SPI1_MOSI | GPIO_SPI1_SCK;
				input = GPIO_SPI1_MISO;
				spi_init = spi_init_master;
			} else {
				output = GPIO_SPI1_MISO;
				nss = GPIO_SPI1_NSS;
				input = GPIO_SPI1_MOSI | GPIO_SPI1_SCK;
				spi_init = spi_init_slave;
			}
		} break;

		case SPI2: {
			gpio_clken = RCC_GPIOB;
			spi_clken = RCC_SPI2;
			irqen = NVIC_SPI2_IRQ;
			if (type == MASTER) {
				output = GPIO_SPI2_NSS | GPIO_SPI2_MOSI | GPIO_SPI2_SCK;
				input = GPIO_SPI2_MISO;
				spi_init = spi_init_master;
			} else {
				output = GPIO_SPI2_MISO;
				input = GPIO_SPI2_NSS | GPIO_SPI2_MOSI | GPIO_SPI2_SCK;
				spi_init = spi_init_slave;
			}
		} break;

		default:
			return; /// @todo Throw an Error (reset?)
	}

	rcc_periph_clock_enable(gpio_clken);
	rcc_periph_clock_enable(spi_clken);

	nvic_enable_irq(irqen);

	gpio_set_mode(
		GPIOB,
		GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		output
	);

	gpio_set_mode(
		GPIOB,
		GPIO_MODE_INPUT,
		GPIO_CNF_INPUT_FLOAT,
		input
	);

	if (type == SLAVE) {
		gpio_set_mode(
			GPIOB,
			GPIO_MODE_INPUT,
			GPIO_CNF_INPUT_FLOAT,
			nss
		);
		gpio_set(GPIOB, nss);
	}

	spi_reset(spi);

	spi_init(
		spi,
		SPI_CR1_BAUDRATE_FPCLK_DIV_256,
		SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
		SPI_CR1_CPHA_CLK_TRANSITION_1,
		SPI_CR1_DFF_16BIT,
		SPI_CR1_MSBFIRST
	);


	spi_disable_software_slave_management(spi);
	spi_enable_ss_output(spi);

	if (type == MASTER) {
		spi_disable_crc(spi);
	} else {
		spi_set_nss_low(spi);
		spi_write(spi, '\0');
		spi_enable_tx_buffer_empty_interrupt(spi);
	}

	spi_enable_rx_buffer_not_empty_interrupt(spi);

	spi_enable(spi);
}

void SPI::send(uint16_t* buffer, size_t len) {
	size_t ii = 0;

	// Fill up the queue by 2 items before enabling the
	// inturrupt
	while (ii < len and ii < 2) {
		BaseType_t r = xQueueSendToBack(this->txq, buffer + ii, 0);
		if (r == pdTRUE) {
			++ii;
		} else {
			taskYIELD();
		}
	}

	spi_enable_tx_buffer_empty_interrupt(this->spi);

	// Fill up the que with the rest of the items
	while (ii < len) {
		BaseType_t r = xQueueSendToBack(this->txq, buffer + ii, 0);
		if (r == pdTRUE) {
			++ii;
		} else {
			taskYIELD();
		}
	}
}

size_t SPI::recv(uint16_t* buffer, size_t len) {
	for (size_t ii = 0; ii < len; ++ii) {
		BaseType_t r = xQueueReceive(this->rxq, (buffer + ii), 0);
		if (r == pdFALSE)
			return ii;
	}
	return len;
}

void SPI::isr() {
	if ((SPI_CR2(spi) & SPI_CR2_TXEIE) != 0 &&
		(SPI_SR(spi) & SPI_SR_TXE) != 0) {
		uint16_t ch = 0x0;

		BaseType_t r = xQueueReceiveFromISR(this->txq, &ch, nullptr);

		if (r == pdFALSE and type == MASTER) {
			spi_disable_tx_buffer_empty_interrupt(spi);
		} else {
			spi_write(spi, ch);
		}
	}

	if ((SPI_CR2(spi) & SPI_CR2_RXNEIE) != 0 &&
		(SPI_SR(spi) & SPI_SR_RXNE) != 0) {
		uint16_t ch = spi_read(spi);
		xQueueSendToBackFromISR(this->rxq, &ch, nullptr);
	}
}

void spi2_isr() {
	spi2->isr();
}

void spi1_isr() {
	spi1->isr();
}

int spi_init_slave(uint32_t spi, uint32_t br, uint32_t cpol, uint32_t cpha, uint32_t dff, uint32_t lsbfirst) {
	(void) br; // unused

	uint32_t reg32 = SPI_CR1(spi);

	/* Reset all bits omitting SPE, CRCEN and CRCNEXT bits. */
	reg32 &= SPI_CR1_SPE | SPI_CR1_CRCEN | SPI_CR1_CRCNEXT;

	reg32 &= ~SPI_CR1_MSTR;	/* Configure SPI as slave. */

	reg32 |= cpol;		/* Set CPOL value. */
	reg32 |= cpha;		/* Set CPHA value. */
	reg32 |= dff;		/* Set data format (8 or 16 bits). */
	reg32 |= lsbfirst;	/* Set frame format (LSB- or MSB-first). */

	//SPI_CR2(spi) |= SPI_CR2_SSOE; /* common case */
	SPI_CR1(spi) = reg32;

	return 0;
}