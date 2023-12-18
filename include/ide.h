#ifndef IDE_H
#define IDE_H

#include "mcp23017-i2c.h"

#define IDE_NDIOW 0 // PC0
#define IDE_NDIOR 0 // PD0
#define IDE_DA0   2 // PD2
#define IDE_DA1   3 // PD3
#define IDE_DA2   4 // PD4
#define IDE_CS0   3 // PC3
#define IDE_CS1   4 // PC4

#define IDE_REG_ERROR   0x09	   //(CS1=0, CS0=1, DA=001)
#define IDE_REG_CYLINDER_LOW  0x0c //(CS1=0, CS0=1, DA=100)
#define IDE_REG_CYLINDER_HIGH 0x0d //(CS1=0, CS0=1, DA=101)
#define IDE_REG_DEVICE  0x0e	   //(CS1=0, CS0=1, DA=110)
#define IDE_REG_STATUS  0x0f	   //(CS1=0, CS0=1, DA=111)
#define IDE_REG_COMMAND 0x0f	   //(CS1=0, CS0=1, DA=111)

// DEVHEAD register
#define IDE_REG_DEVICE_DEV 0x10

// STATUS register
#define IDE_REG_STATUS_BUSY 0x80

#define IDE_REG_COMMAND_DIAG 0x90

// enable GPIO used
void ide_setup_pins(void) {
	// Enable GPIOs
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;

	// GPIO C0
	GPIOC->CFGLR &= ~(0xf << (4 * IDE_NDIOW));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * IDE_NDIOW);

	// GPIO D0
	GPIOD->CFGLR &= ~(0xf << (4 * IDE_NDIOR));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * IDE_NDIOR);

	// GPIO D2
	GPIOD->CFGLR &= ~(0xf << (4 * IDE_DA0));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * IDE_DA0);

	// GPIO D3
	GPIOD->CFGLR &= ~(0xf << (4 * IDE_DA1));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * IDE_DA1);

	// GPIO D4
	GPIOD->CFGLR &= ~(0xf << (4 * IDE_DA2));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * IDE_DA2);

	// GPIO C3
	GPIOC->CFGLR &= ~(0xf << (4 * IDE_CS0));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * IDE_CS0);

	// GPIO C4
	GPIOC->CFGLR &= ~(0xf << (4 * IDE_CS1));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * IDE_CS1);
}


void ide_set_reg_addr(uint8_t reg) {
	uint8_t tmp_reg;
	tmp_reg = GPIOD->OUTDR & ~((1 << IDE_DA0) | (1 << IDE_DA1) | (1 << IDE_DA2));
	tmp_reg |= (reg & 7) << IDE_DA0;
	GPIOD->OUTDR = tmp_reg;
	tmp_reg = GPIOC->OUTDR & ~((1 << IDE_CS0) | (1 << IDE_CS1));
	tmp_reg |= (((~reg) >> 3) & 3) << IDE_CS0;
	GPIOC->OUTDR = tmp_reg;
}

// write/read pins
void ide_nDIOR_on(void) {
	GPIOD->BSHR = (1 << (16 + IDE_NDIOR)); // nDIOR -> clear bit
}

void ide_nDIOR_off(void) {
	GPIOD->BSHR = (1 << IDE_NDIOR); // nDIOR -> set bit
}

void ide_nDIOW_on(void) {
	GPIOC->BSHR = (1 << (16 + IDE_NDIOW)); // nDIOW -> clear bit
}

void ide_nDIOW_off(void) {
	GPIOC->BSHR = (1 << IDE_NDIOW); // nDIOW -> set bit
}

// switch TS (Tri-state) pins to high impedance state
void ide_turn_pins_safe(void) {
	// 1 - input
	// byte mode + no sequential = toggles between registers pair
	static uint8_t const to_input[] = {0xff, 0xff};
	ide_nDIOW_off();
	ide_nDIOR_off();
	mcp23017_i2c_send(MCP23017_I2C_ADDR, MCP23017_IODIRA, to_input, sizeof(to_input));
}

// switch to outputs
void ide_turn_pins_output(void) {
	// 0 - output
	// byte mode + no sequential = toggles between registers pair
	static uint8_t const to_output[] = {0x0, 0x0};
	mcp23017_i2c_send(MCP23017_I2C_ADDR, MCP23017_IODIRA, to_output, sizeof(to_output));
}

// IDE read
uint16_t ide_read(uint8_t reg) {
	uint16_t data;

	ide_set_reg_addr(reg);
	Delay_Tiny(10);

	ide_nDIOR_on();
	Delay_Us(1);

	mcp23017_i2c_recv(MCP23017_I2C_ADDR, MCP23017_GPIOA, (uint8_t*)&data, sizeof(data));

	ide_nDIOR_off();
	return(data);
}

// IDE write
void ide_write(uint8_t reg, uint16_t data) {
	ide_turn_pins_output();
	ide_set_reg_addr(reg);
	mcp23017_i2c_send(MCP23017_I2C_ADDR, MCP23017_GPIOA, (uint8_t*)&data, sizeof(data));
	Delay_Tiny(10);

	ide_nDIOW_on();
	Delay_Us(1);

	ide_turn_pins_safe();
	Delay_Us(1);
	return(data);
}


// status registr subs
uint8_t ide_not_busy(void)
{
	return !(ide_read(IDE_REG_STATUS) & IDE_REG_STATUS_BUSY);
}


#endif // IDE_H
