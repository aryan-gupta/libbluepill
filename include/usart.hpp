#pragma once

#include <FreeRTOS.h>
#include <queue.h>

class USART {
	QueueHandle_t txq;
	QueueHandle_t rxq;
	uint32_t usart;

	static void usart_setup();

public:
	USART(uint32_t usart_);
	void send(char* buffer);
	void send(char* buffer, size_t len);
	size_t recv(char* buffer, size_t len);

	void isr();
};
