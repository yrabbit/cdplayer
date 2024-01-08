#ifndef ADC_H
#define ADC_H

#define ADC_NUMCHLS 2
volatile uint16_t adc_buffer[ADC_NUMCHLS];

void adc_init(void) {
	// ADCCLK = 24 MHz => RCC_ADCPRE = 0: divide by 2
	RCC->CFGR0 &= ~(0x1F<<11);

	// Enable GPIOD and ADC
	RCC->APB2PCENR |=	RCC_APB2Periph_GPIOD | RCC_APB2Periph_ADC1;

	// PD5 is analog input chl 5
	GPIOD->CFGLR &= ~(0xf << (4 * 5));	// CNF = 00: Analog, MODE = 00: Input

	// PD6 is analog input chl 6
	GPIOD->CFGLR &= ~(0xf << (4 * 6));	// CNF = 00: Analog, MODE = 00: Input

	// Reset the ADC to init all regs
	RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;

	// Set up two conversions on chl 6 and 5
	ADC1->RSQR1 = (ADC_NUMCHLS - 1) << 20;	// two chls in the sequence
	ADC1->RSQR2 = 0;
	ADC1->RSQR3 = (5 << ( 5 * 0)) | (6 << (5 * 1));
	//ADC1->RSQR3 = (5 << ( 5 * 0));
	//ADC1->RSQR3 = (6 << ( 5 * 0));

	// set sampling time for chl 6, 5
	// 0:7 => 3/9/15/30/43/57/73/241 cycles
	ADC1->SAMPTR2 = (7 << (3 * 6)) | (7 << (3 * 5));
	//ADC1->SAMPTR2 = (7 << (3 * 5));
	//ADC1->SAMPTR2 = (7 << (3 * 6));

	// turn on ADC
	ADC1->CTLR2 |= ADC_ADON;

	// Reset calibration
	ADC1->CTLR2 |= ADC_RSTCAL;
	while(ADC1->CTLR2 & ADC_RSTCAL);

	// Calibrate
	ADC1->CTLR2 |= ADC_CAL;
	while(ADC1->CTLR2 & ADC_CAL);

	// Turn on DMA
	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

	//DMA1_Channel1 is for ADC
	DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
	DMA1_Channel1->MADDR = (uint32_t)adc_buffer;
	DMA1_Channel1->CNTR  = ADC_NUMCHLS;
	DMA1_Channel1->CFGR  =
		DMA_M2M_Disable |
		DMA_Priority_VeryHigh |
		DMA_MemoryDataSize_HalfWord |
		DMA_PeripheralDataSize_HalfWord |
		DMA_MemoryInc_Enable |
		DMA_Mode_Circular |
		DMA_DIR_PeripheralSRC;

	// Turn on DMA channel 1
	DMA1_Channel1->CFGR |= DMA_CFGR1_EN;

	// enable scanning
	ADC1->CTLR1 |= ADC_SCAN;

	// Enable continuous conversion and DMA
	ADC1->CTLR2 |= ADC_CONT | ADC_DMA | ADC_EXTSEL;

	// start conversion
	ADC1->CTLR2 |= ADC_SWSTART;
}

#endif // ADC_H


