
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/cm3/nvic.h>

#include "i2c.hpp"
#include "gpio.hpp"

static void test_delay(unsigned int t) {
	unsigned long long time = t * 1'000'000;
	for (int i = 0; i < time; ++i) {
		__asm__("nop");
	}
}

void i2c2_er_isr() {
	while (I2C_SR1(I2C2) & I2C_SR1_BERR)
		I2C_SR1(I2C2) &= ~(I2C_SR1_BERR);

	volatile uint16_t a = I2C_SR1(I2C2);
	volatile uint16_t b = 5;
	while (true) {
		b = 5;
	}

}

I2C::I2C(uint32_t i2c_, i2c_speeds speed)
: i2c{ i2c_ } {
	i2c_reset(i2c);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_AFIO);

	nvic_enable_irq(NVIC_I2C2_ER_IRQ);

	rcc_periph_clken rcc_clk;
	uint16_t clock_pin;
	uint16_t data_pin;

	switch(i2c) {
		case I2C1:
			rcc_clk = RCC_I2C1;
			clock_pin = GPIO_I2C1_SDA;
			data_pin = GPIO_I2C1_SCL;
		break;

		case I2C2:
			rcc_clk = RCC_I2C2;
			clock_pin = GPIO_I2C2_SDA;
			data_pin = GPIO_I2C2_SCL;
		break;

		default:
			return; // @todo Throw an Error here
	};

	rcc_periph_clock_enable(rcc_clk);

	gpio_set_mode(
		GPIOB,
		GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
		clock_pin | data_pin
	);
	gpio_set(GPIOB, clock_pin | data_pin);

	i2c_peripheral_disable(i2c);
	i2c_reset(i2c);
	i2c_set_speed(i2c, speed, I2C_CR2_FREQ_36MHZ);
	i2c_set_own_7bit_slave_address(i2c,0x23);
	I2C_CR1(i2c) &= ~I2C_CR1_NOSTRETCH;
	i2c_peripheral_enable(i2c);

	i2c_enable_interrupt(i2c, I2C_CR2_ITERREN);
}

void I2C::wait_not_busy() {
	// wait while the bus is busy
	while ( I2C_SR2(i2c) & I2C_SR2_BUSY );
}

void I2C::send_start() {
	i2c_send_start(i2c);

	// wait until start bit is sent, we are in master mode
	// and bus isnt busy
	//while (! (I2C_SR1(i2c) & I2C_SR1_SB));

	while ( !((I2C_SR1(i2c) & I2C_SR1_SB)
	  && (I2C_SR2(i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))) );
}

void I2C::write_addr(uint8_t addr, uint8_t rw) {
	if (rw == I2C_READ)
		i2c_enable_ack(i2c);

	i2c_send_7bit_address(i2c,addr, rw);

	// wait until address is sent
	while ( !(I2C_SR1(i2c) & I2C_SR1_ADDR) );

	// clear any flags set
	(void)I2C_SR2(i2c);
}

void I2C::write(uint8_t byte) {
	i2c_send_data(i2c, byte);

	// wait until byte transfer is finished
	while ( !(I2C_SR1(i2c) & (I2C_SR1_BTF)) );
}

uint8_t I2C::read() {
	// wait until byte is recived
	while ( !(I2C_SR1(i2c) & I2C_SR1_RxNE) );

	return i2c_get_data(i2c);
}

uint8_t I2C::read_nack() {
	// disabling ack and senting stop bit must be
	// set before reciving the last byte
	i2c_disable_ack(i2c);
	i2c_send_stop(i2c);

	// wait until transfer is complete
	while ( !(I2C_SR1(i2c) & I2C_SR1_RxNE) );

	return i2c_get_data(i2c);
}

uint8_t I2C::read_reg8(uint8_t addr, uint8_t reg) {
	wait_not_busy();
	send_start();
	write_addr(addr, I2C_WRITE);
	write(reg);

	send_start(); // repeated start
	write_addr(addr, I2C_READ);
	return read_nack();
}

void I2C::write_reg(uint8_t addr, uint8_t reg, uint8_t value) {
	wait_not_busy();
	send_start();
	write_addr(addr, I2C_WRITE);
	write(reg);
	write(value);
	i2c_send_stop(i2c);
}