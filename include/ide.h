#ifndef IDE_H
#define IDE_H

#include "mcp23017-i2c.h"

#define NDIOR 0 // PC0
#define NDIOW 0 // PD0
#define DA0   2 // PD2
#define DA1   3 // PD3
#define DA2   4 // PD4

// enable GPIO used
void ide_setup_pins(void) {
	// Enable GPIOs
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;

	// GPIO C0
	GPIOC->CFGLR &= ~(0xf << (4 * NDIOR));
	GPIOC->CFGLR |= GPIO_CNF_IN_FLOATING << (4 * NDIOR);

	// GPIO D0
	GPIOD->CFGLR &= ~(0xf << (4 * NDIOW));
	GPIOD->CFGLR |= GPIO_CNF_IN_FLOATING << (4 * NDIOW);

	// GPIO D2
	GPIOD->CFGLR &= ~(0xf << (4 * DA0));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * DA0);

	// GPIO D3
	GPIOD->CFGLR &= ~(0xf << (4 * DA1));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * DA1);

	// GPIO D4
	GPIOD->CFGLR &= ~(0xf << (4 * DA2));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * DA2);
}

// switch TS (Tri-state) pins to high impedance state
void ide_turn_pins_safe(void) {
	// 1 - input
	// byte mode + no sequential = toggles between registers pair
	static uint8_t const to_input[] = {MCP23017_IODIRA, 0xff, 0xff};
	mcp23017_i2c_send(MCP23017_I2C_ADDR, to_input, sizeof(to_input));

	// GPIO C0
	GPIOC->CFGLR &= ~(0xf << (4 * NDIOR));
	GPIOC->CFGLR |= GPIO_CNF_IN_FLOATING << (4 * NDIOR);

	// GPIO D0
	GPIOD->CFGLR &= ~(0xf << (4 * NDIOW));
	GPIOD->CFGLR |= GPIO_CNF_IN_FLOATING << (4 * NDIOW);

}

void ide_set_reg_addr(uint8_t reg) {
	GPIOD->OUTDR &= ~((1 << DA0) | (1 << DA1) | (1 << DA2));
	GPIOD->OUTDR |= (reg & 7) << DA0;
}

// write/read pins
void ide_nDIOW_on(void) {
	GPIOD->CFGLR &= ~(0xf << (4 * NDIOW));
	GPIOD->BSHR = (1 << (16 + NDIOW)); // nDIOW -> clear bit
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * NDIOW);
}

void ide_nDIOW_off(void) {
	GPIOD->CFGLR &= ~(0xf << (4 * NDIOW));
	GPIOD->BSHR = (1 << NDIOW); // nDIOW -> set bit
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * NDIOW);
}

void ide_nDIOR_on(void) {
	GPIOC->CFGLR &= ~(0xf << (4 * NDIOR));
	GPIOC->BSHR = (1 << (16 + NDIOR)); // nDIOR -> clear bit
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * NDIOR);
}

void ide_nDIOR_off(void) {
	GPIOC->CFGLR &= ~(0xf << (4 * NDIOR));
	GPIOC->BSHR = (1 << NDIOR); // nDIOR -> set bit
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * NDIOR);
}

// IDE read
uint16_t ide_read (uint8_t reg) {
	ide_set_reg_addr(reg);
	ide_nDIOR_on();
	ide_turn_pins_safe();
	/*
  reg = regval & B01111111;
  Wire.beginTransmission(RegSel);
  Wire.send(reg);
  Wire.endTransmission();
  Wire.requestFrom(DataH, 1);
  dataHval = Wire.receive();
  Wire.requestFrom(DataL, 1);
  dataLval = Wire.receive();
  highZ();
  */
	return(0);
}

#endif // IDE_H
