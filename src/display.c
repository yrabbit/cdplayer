/* High level display driver (video memory and stuff) */
#include "ch32v003fun.h"

#include <stdlib.h>
#include <string.h>

#include "spi.h"
#include "lcd.h"
#include "display.h"

//#define DMA
#ifdef DMA
	#include "dma.h"
#endif

uint8_t vram[SCREEN_HEIGHT][SCREEN_WIDTH];
uint8_t dirty_page;

/* transfer routines */
static void send_page(int page) {
	lcd_set_page_column(page, 0);
	lcd_data_start();
	for (int col = 0; col < SCREEN_WIDTH; ++col) {
		spi_send8(vram[page][col]);
	}
	lcd_data_stop();
}

/* interface */
void refresh_screen(void) {
	for(int page = 0; page < SCREEN_HEIGHT; ++page) {
		if (dirty_page & (1 << page)) {
			send_page(page);
		}
	}
}

void clear_screen(void) {
	for (int page = 0; page < SCREEN_HEIGHT; ++page) {
		memset(vram[page], 0, SCREEN_WIDTH);
	}
	dirty_page = 0xff;
}

void init_screen(void) {
	dirty_page = 0;
}
