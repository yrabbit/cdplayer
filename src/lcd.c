/* low level LCD driver */
#include "ch32v003fun.h"

#include "spi.h"
#include "lcd.h"

static inline void gpio_init(void) {
	// LCD_NRESET_PIN    - 2MHz PP
	// LCD_DATA_CMD_PIN  - 10MHz PP
	// LCD_BACKLIGHT_PIN - 2MHz PP
	GPIOC->CFGLR &= ~(0xf << (4 * LCD_NRESET_PIN) | 0xf << (4 * LCD_DATA_CMD_PIN)
			        | 0xf << (4 * LCD_BACKLIGHT_PIN));
	GPIOC->CFGLR |= (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4 * LCD_NRESET_PIN)
				  | (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * LCD_DATA_CMD_PIN)
				  | (GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (4 * LCD_BACKLIGHT_PIN);
}

static inline void nreset_low(void) {
	GPIOC->BSHR = (1 << (16 + LCD_NRESET_PIN));
}

static inline void nreset_high(void) {
	Delay_Ms(1);
	GPIOC->BSHR = (1 << LCD_NRESET_PIN);
	Delay_Ms(1);
}

void lcd_data_start(void) {
	spi_not_busy_wait();
	GPIOC->BSHR = (1 << LCD_DATA_CMD_PIN);
}

void lcd_data_stop(void) {
	spi_not_busy_wait();
	GPIOC->BCR = (1 << LCD_DATA_CMD_PIN);
}

void lcd_init(void) {
	gpio_init();
	// reset the display ICs
	nreset_high();
	Delay_Us(15);
	nreset_low();
	Delay_Us(100);
	nreset_high();

	// init seq
	spi_nss_low();   // permanently select screen device
	spi_send8(0xa0); // not 180 h-rotation
	spi_send8(0x2f); // all Power Unit on
	spi_send8(0x27); // voltage resistor 3 (0..7)
	spi_send8(0x81); // contrast
	spi_send8(15);   // contrast value
	spi_send8(0xaf); // display on
}

void lcd_set_page_column(int page, int col) {
	spi_send8(0xb0 | (page & 0xf));
	spi_send8(0x10 | (col >> 4));
	spi_send8(col & 0xf);
}

void lcd_sleep(void) {
}

void lcd_wakeup(void) {
}

