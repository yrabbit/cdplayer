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

// Protocol
#define PROTO_CMD_NOP               0x00
#define PROTO_CMD_RESET             0x01
#define PROTO_CMD_GET_MODEL         0x02
#define PROTO_CMD_EJECT             0x03
#define PROTO_CMD_GET_DISK          0x04
#define PROTO_CMD_GET_TRACK_CNT     0x05
#define PROTO_CMD_GET_TRACK_DESC    0x06
#define PROTO_CMD_PLAY_TRACK        0x07
#define PROTO_CMD_GET_STATUS        0x08
#define PROTO_CMD_PAUSE_PLAY        0x09
#define PROTO_CMD_RESUME_PLAY       0x0a

#define PROTO_NO_PLAYER_DATA		0x00
#define PROTO_HAS_DATA        		0x80

// open/load during eject
#define PROTO_EJECT_OPEN            ((uint8_t)'O')
#define PROTO_EJECT_LOAD            ((uint8_t)'L')

// disk status
#define PROTO_GET_DISK_AUDIO        ((uint8_t)'A')
#define PROTO_GET_DISK_OPEN         ((uint8_t)'O')
#define PROTO_GET_DISK_UNK          ((uint8_t)'U')

// audio status


#define MAKE_ANSWER(len) (PROTO_HAS_DATA | len)

// =========================================================================
// IO queue
#define IO_QUEUE_LEN  128  // 2^x only
#define IO_QUEUE_MASK (IO_QUEUE_LEN - 1)
typedef struct {
	uint32_t read, write;
	uint8_t storage[IO_QUEUE_LEN];
} msg_queue_t;

uint8_t read_byte_from_queue(volatile msg_queue_t * const q) {
	return (q->storage[(q->read++) & IO_QUEUE_MASK]);
}

void write_byte_to_queue(volatile msg_queue_t * const q, uint8_t byte) {
	q->storage[(q->write++) & IO_QUEUE_MASK] = byte;
}

void empty_queue(volatile msg_queue_t * const q) {
	q->write = q->read;
}

int8_t queue_empty(volatile msg_queue_t * const q) {
	return (q->read == q->write);
}

int8_t queue_full(volatile msg_queue_t * const q) {
	return ((q->read & IO_QUEUE_MASK) == ((q->write + 1) & IO_QUEUE_MASK));
}
// =========================================================================

volatile struct {
	uint8_t in_byte;  // host->player byte
	uint8_t out_byte; // player->host byte
	uint8_t bit_cnt;  // current bit
	msg_queue_t in;
	msg_queue_t out;
} xfc_data = {0, 0, 0, {0, 0, {0}}, {0, 0, {0}}};

void refill_xfc_out_data(void) {
	if (queue_empty(&xfc_data.out)) {
		xfc_data.out_byte = PROTO_NO_PLAYER_DATA;
	} else {
		xfc_data.out_byte = read_byte_from_queue(&xfc_data.out);
	}
}

void reset_xfc_in_data(void) {
	xfc_data.bit_cnt = 0;
	empty_queue(&xfc_data.in);
}

void reset_xfc_out_data(void) {
	xfc_data.out_byte = 0;
	empty_queue(&xfc_data.out);
}

void EXTI7_0_IRQHandler(void) __attribute__((interrupt));
void EXTI7_0_IRQHandler(void) {
	// reset "watchdog"
	TIM2->CNT = 0;

	uint8_t bit;
	// falling edge
#ifdef XFC_INV
	if (GPIOC->INDR & (1 << XFC_CLK_PIN)) {
#else
	if (!(GPIOC->INDR & (1 << XFC_CLK_PIN))) {
#endif
		// next bit
		if (++xfc_data.bit_cnt == 8) {
			xfc_data.bit_cnt = 0;
			refill_xfc_out_data();
		}

		bit = xfc_data.out_byte & 1;
		xfc_data.out_byte >>= 1;

#ifdef XFC_INV
		// send player bit
		if (!bit) {
#else
		// send player bit
		if (bit) {
#endif
				GPIOC->BSHR = (1 << XFC_OUT_PIN);
		} else {
				GPIOC->BSHR = 1 << (16 + XFC_OUT_PIN);
		}
	} else {
#ifdef XFC_INV
		// read host bit
		bit = 1 - ((GPIOC->INDR >> XFC_IN_PIN) & 1);
#else
		// read host bit
		bit = (GPIOC->INDR >> XFC_IN_PIN) & 1;
#endif
		xfc_data.in_byte >>= 1;
		xfc_data.in_byte  |= (bit << 7);
		if (xfc_data.bit_cnt == 7) { // on rising edge - this allows fast cmd handle
			write_byte_to_queue(&xfc_data.in, xfc_data.in_byte);
		}
	}
	// clear the interrupt
	EXTI->INTFR = 1 << XFC_CLK_PIN;
}


// "watchdog" timer
void TIM2_IRQHandler(void) __attribute__((interrupt));
void TIM2_IRQHandler(void) {
	// if timeout inside the byte or the host does not require data for a long time
	if (xfc_data.bit_cnt || !queue_empty(&xfc_data.out)) {
		reset_xfc_in_data();
		reset_xfc_out_data();
	}
	TIM2->INTFR &= ~TIM_FLAG_Update;
}

void host_xface_init(void) {
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
