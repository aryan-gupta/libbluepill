#pragma once

#include <new>

#include <FreeRTOS.h>
#include <task.h>

#include <libopencm3/cm3/nvic.h>

enum class CAN_ID {
	DEBUG_DEPLOYUC = 0x8,
	DEBUG_ODROID = 0x9,
	DEBUG_UASUC = 0xA,
	DEBUG_NANOPI = 0xB,
	DEBUG_2PUC = 0xC,

	DEPLOY_NOW = 0x10,
	LIFT_OFF = 0x20,
	DEPLOY_ACK = 0x40,
	DEPLOY_METHOD = 0x50,
	SRA_GPS_LOC = 0x80,
	ALT = 0x90,

	CMD_REPLY_DEPLOYUC = 0x100,
	CMD_REPLY_ODROID = 0x101,
	CMD_REPLY_UASUC = 0x102,
	CMD_REPLY_NANOPI = 0x103,
	CMD_REPLY_2PUC = 0x104,

	CMD_DEPLOYUC = 0x110,
	CMD_ODROID = 0x111,
	CMD_UASUC = 0x112,
	CMD_NANOPI = 0x113,
	CMD_2PUC = 0x114,

	HEARTBEAT_DEPLOYUC = 0x200,
	HEARTBEAT_ODROID = 0x201,
	HEARTBEAT_UASUC = 0x202,
	HEARTBEAT_NANOPI = 0x203,
	HEARTBEAT_2PUC = 0x204,

	GENERAL_DEPLOYUC = 0x400,
	GENERAL_ODROID = 0x401,
	GENERAL_UASUC = 0x402,
	GENERAL_NANOPI = 0x403,
	GENERAL_2PUC = 0x404,

	DEVICE_MASK = 0x7,
	MSG_TYPE_MASK = 0x78F,

	CAN_ID_MASK = 0x7FF
};

constexpr uint8_t MAX_SPI_MSG_LEN = 8;
constexpr uint8_t MAX_CAN_MSGS = 4;
constexpr uint16_t SPI_HEAD = 0xA4;
constexpr uint16_t SPI_TAIL = 0x2555;

void vPortSVCHandler( void ) __attribute__ (( naked ));
void xPortPendSVHandler( void ) __attribute__ (( naked ));
void xPortSysTickHandler( void );
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);

template <typename T, typename... Args>
T* create(Args... args) {
	T* ptr = (T*) pvPortMalloc(sizeof(T));
	new (ptr) T{ std::forward<Args>(args)... };
	return ptr;
}
