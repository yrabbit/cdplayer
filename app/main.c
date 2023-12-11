#include "ch32v003fun.h"

#include "spi.h"
#include "lcd.h"
#include "display.h"

int main()
{
	SystemInit();

	spi_init();
	lcd_init();
	init_screen();

	clear_screen();
	refresh_screen();

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
}

