#pragma once

#include <stdint.h>
#include <string.h>

#include <FreeRTOS.h>
#include <queue.h>

/// A simple class to handle all CAN messages
class CAN {
	QueueHandle_t txq; /// buffer for transmitting messages
	QueueHandle_t rxq; /// buffer for recived messages

public:
	struct tx_frame {
		uint32_t id;
		bool xid;
		bool rtr;
		uint8_t len;
		uint8_t data[8];
	};

	struct rx_frame {
		uint8_t fifo; // which fifo

		uint32_t id; // arbritation ID
		bool xid; // is extended ID
		bool rtr; // request transmit
		uint8_t fmi; // filter match index
		uint8_t len;
		uint8_t data[8];
	};


	CAN(bool altcfg = false);

	void send(tx_frame* buffer, size_t len = 1);
	size_t recv(rx_frame* buffer, size_t len = 1);

	void rx_isr(uint8_t fifo, uint8_t num_msg);
	void tx_isr();
};