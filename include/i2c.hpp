#pragma once

#include <array>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

class I2C {
	uint32_t i2c;

	void wait_not_busy();
	void send_start();
	void write_addr(uint8_t addr, uint8_t rw);
	void write(uint8_t byte);
	uint8_t read();
	uint8_t read_nack();

public:
	I2C(uint32_t i2c_, i2c_speeds speed);

	void write_reg(uint8_t addr, uint8_t reg, uint8_t value);

	uint8_t read_reg8(uint8_t addr, uint8_t reg);
	uint16_t read_reg16(uint8_t addr, uint8_t reg);

	template <size_t N>
	std::array<uint8_t, N> read_block_reg8(uint8_t addr, uint8_t start_reg);

	template <size_t N>
	std::array<uint16_t, N> read_block_reg16(uint8_t addr, uint8_t start_reg);
};