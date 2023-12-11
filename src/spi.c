// SPI interface
// YRabbit 2023

#include "ch32v003fun.h"
#include "spi.h"

void spi_setup(void) {
	// Reset SPI1 to init all regs
	RCC->APB2PRSTR |= RCC_APB2Periph_SPI1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_SPI1;

	// BR[2:0] = 000  FCLK/2 = 24MHz
	// CPOL = 1 CPHA = 1
	// DFF = 8 bit 0
	// BIDMODE = 1  1-line bidirectional
	// BIDIOE  = 1  transmist only
	// MSTR = 1 master
	// SSOE = 1 enable NSS
	SPI1->CTLR1 = SPI_CTLR1_BIDIMODE | SPI_CTLR1_BIDIOE | SPI_CTLR1_CPOL | SPI_CTLR1_CPHA;
    SPI1->CTLR2 = SPI_CTLR2_SSOE;
	SPI1->CTLR1 |= SPI_CTLR1_MSTR;
	SPI1->CTLR1 &= ~SPI_CTLR1_SPE;
}

void spi_init(void) {
	// all spi pins are PC, enable clock for them and for SPI
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_SPI1;
	// PC1 - NSS 10MHz  alternate PP
	// PC5 - SCK 50MHz  alternate PP
	// PC6 - MOSI 50MHz alternate PP
	GPIOC->CFGLR &= ~(0xf << (4 * 1) | 0xf << (4 * 5) | 0xf << (4 * 6));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF) << (4 * 1)
				  | (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF) << (4 * 5)
				  | (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF) << (4 * 6);
	spi_setup();
}

void spi_nss_low(void) {
	Delay_Us(150);
	SPI1->CTLR1 |= SPI_CTLR1_SPE;
}

void spi_not_busy_wait(void) {
	while (SPI1->STATR & SPI_STATR_BSY);
}

void spi_nss_high(void) {
	spi_not_busy_wait();
	Delay_Us(50);
	SPI1->CTLR1 &= ~SPI_CTLR1_SPE;
}

void spi_send8(uint8_t byte) {
		spi_not_busy_wait();
		SPI1->DATAR = byte;
}
