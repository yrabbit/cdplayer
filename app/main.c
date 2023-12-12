#include "ch32v003fun.h"

#include <stdio.h>

#include "mcp23017-i2c.h"

int main()
{
	SystemInit();

	printf("CFGR0:%lx\n\r", RCC->CFGR0);
	mcp23017_i2c_setup();
	uint8_t const all_out[] = {0, 0};
	uint8_t neg_pins[] = {0x14, 0xff};
	mcp23017_i2c_send(MCP23017_I2C_ADDR, all_out, 2);


	uint8_t bit = 1;
	uint8_t dir = 1;
	while (1) {
		if (dir == 1) {
			bit <<= 1;
		} else {
			bit >>= 1;
		}
		neg_pins[1] = (0xff - bit);
		mcp23017_i2c_send(MCP23017_I2C_ADDR, neg_pins, 2);
		Delay_Ms(250);
		if (bit == 0x80 || bit == 1) {
			dir = -dir;
		}
	}

	/*
	// two buttons D2 and D3
#define BOTTOM_KEY 2
#define TOP_KEY 3
	GPIOD->CFGLR &= (0xf << (4 * BOTTOM_KEY) | 0xf << (4 * TOP_KEY));
	GPIOD->CFGLR |= (GPIO_CNF_IN_PUPD << (4 * BOTTOM_KEY))
		           |(GPIO_CNF_IN_PUPD << (4 * TOP_KEY));
	// pullup
	GPIOD->OUTDR |= (1 << BOTTOM_KEY) | (1 << TOP_KEY);

	enum {st_norm = 0, st_sleep = 1} state = st_norm;

#define READ_PORT(pin) (GPIOD->INDR & (1 << pin))

	while (1) {
		switch (state) {
			case st_norm:
				if (READ_PORT(BOTTOM_KEY)) {
					state = st_sleep;
					lcd_sleep();
				}
				break;
			case st_sleep:
				if (READ_PORT(TOP_KEY)) {
					state = st_norm;
					lcd_wakeup();
					init_screen();
					clear_screen();
					refresh_screen();
				}
				break;
			default:
				break;
		}
	}
	*/
}

