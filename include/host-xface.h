#ifndef HOST_XFACE_H
#define HOST_XFACE_H

#include "ch32v003fun.h"

/***********************************************
 * Interface with the host computer.
 * 1 - CLK  host -> player
 * 2 - IN   host -> player
 * 3 - OUT  player -> host
 * 4 - GND
 ***********************************************/

#define XFC_CLK_PIN 5 // PC5
#define XFC_IN_PIN  6 // PC6
#define XFC_OUT_PIN 7 // PC7

// host uses inversion on external interface
#define XFC_INV

// bit sent and bit read
volatile struct {
	uint8_t ready : 1;
	uint8_t out : 1;
	uint8_t in : 1;
} xfc_bits = {0, 0, 0};

void EXTI7_0_IRQHandler(void) __attribute__((interrupt));
void EXTI7_0_IRQHandler(void) {
#ifdef XFC_INV
	// read host bit
	xfc_bits.in = 1 - ((GPIOC->INDR >> XFC_IN_PIN) & 1);
	// send player bit
	if (!xfc_bits.out) {
#else
	// read host bit
	xfc_bits.in = (GPIOC->INDR >> XFC_IN_PIN) & 1;
	// send player bit
	if (xfc_bitx.out) {
#endif
			GPIOC->BSHR = (1 << XFC_OUT_PIN);
	} else {
			GPIOC->BSHR = 1 << (16 + XFC_OUT_PIN);
	}
	// falling edge
#ifdef XFC_INV
	if (GPIOC->INDR & (1 << XFC_CLK_PIN)) {
#else
	if (!(GPIOC->INDR & (1 << XFC_CLK_PIN))) {
#endif
		xfc_bits.ready = 1;
	}

	// clear the interrupt
	EXTI->INTFR = 1 << XFC_CLK_PIN;
}

volatile uint16_t tim = 0;
// "watchdog" timer
void TIM2_IRQHandler(void) __attribute__((interrupt));
void TIM2_IRQHandler(void) {
	tim++;
	TIM2->INTFR &= ~TIM_FLAG_Update;
}

void host_xface_init(void) {
	 xfc_bits.ready = 0;

	// set pin modes
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO;
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

	// clk
	GPIOC->CFGLR &= ~(0xf << (4 * XFC_CLK_PIN));	// CNF = 00: Analog, MODE = 00: Input
	GPIOC->CFGLR |= GPIO_CNF_IN_PUPD << (4 * XFC_CLK_PIN); // CNF = 10: With pullup/pulldown
	GPIOC->OUTDR |= 1 << XFC_CLK_PIN; // pullup

	// in
	GPIOC->CFGLR &= ~(0xf << (4 * XFC_IN_PIN));	// CNF = 00: Analog, MODE = 00: Input
	GPIOC->CFGLR |= GPIO_CNF_IN_PUPD << (4 * XFC_IN_PIN); // CNF = 10: With pullup/pulldown
	GPIOC->OUTDR |= 1 << XFC_IN_PIN; // pullup

	// out
    GPIOC->CFGLR &= ~(0xfU << (4 * XFC_OUT_PIN));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * XFC_OUT_PIN);

	// CLK pin interrupt
	AFIO->EXTICR = 2 << (2 * XFC_CLK_PIN); // PINC
	EXTI->INTENR = 1 << XFC_CLK_PIN;  // enable interrupt
	EXTI->RTENR  = 1 << XFC_CLK_PIN;  // on rising edge
	EXTI->FTENR  = 1 << XFC_CLK_PIN;  // on falling edge
	NVIC_EnableIRQ(EXTI7_0_IRQn);

	// "watchdog"
	RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;
	uint16_t tmpcr1 = 0;
    tmpcr1 = TIM2->CTLR1;

	tmpcr1 &= ~(TIM_DIR | TIM_CMS);
	tmpcr1 |= TIM_CounterMode_Up;

    tmpcr1 &= ~TIM_CTLR1_CKD;
    tmpcr1 |= TIM_CKD_DIV1;

    TIM2->CTLR1 = tmpcr1;
    TIM2->ATRLR = 48000 - 1; // (48000 - 1)/48MHz * 2000 = 2 sec
    TIM2->PSC = 1000 - 1;
	TIM2->DMAINTENR |= TIM_FLAG_Update;
    TIM2->SWEVGR = TIM_PSCReloadMode_Immediate;

	NVIC_EnableIRQ(TIM2_IRQn);
	TIM2->CTLR1 |= TIM_CEN;
}

#endif // HOST_XFACE_H
