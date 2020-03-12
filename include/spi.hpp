#pragma once

#include <cstddef>

#include <FreeRTOS.h>
#include <queue.h>

class SPI {
public:
	enum SPI_TYPE {
		SLAVE,
		MASTER
	};

private:
	QueueHandle_t txq;
	QueueHandle_t rxq;

	SPI_TYPE type;
	uint32_t spi;

public:


	SPI(uint32_t spi_, SPI_TYPE type_);

	void send(uint16_t* buffer, size_t len = 1);
	size_t recv(uint16_t* buffer, size_t len = 1);

	void isr();
};

int spi_init_slave(uint32_t spi, uint32_t br, uint32_t cpol, uint32_t cpha, uint32_t dff, uint32_t lsbfirst);
