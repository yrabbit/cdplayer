#ifndef IDE_H
#define IDE_H

#include "mcp23017-i2c.h"

#define IDE_TIMEOUT 600 // ms

#define IDE_NDIOW 0 // PC0
#define IDE_NDIOR 0 // PD0
#define IDE_DA0   2 // PD2
#define IDE_DA1   3 // PD3
#define IDE_DA2   4 // PD4
#define IDE_CS0   3 // PC3
#define IDE_CS1   4 // PC4

#define IDE_REG_DATA    0x00       //(CS1=0, CS0=1, DA=000)
#define IDE_REG_ERROR   0x09	   //(CS1=0, CS0=1, DA=001)
#define IDE_REG_CYLINDER_LOW  0x0c //(CS1=0, CS0=1, DA=100)
#define IDE_REG_CYLINDER_HIGH 0x0d //(CS1=0, CS0=1, DA=101)
#define IDE_REG_DEVICE  0x0e	   //(CS1=0, CS0=1, DA=110)
#define IDE_REG_STATUS  0x0f	   //(CS1=0, CS0=1, DA=111)
#define IDE_REG_COMMAND 0x0f	   //(CS1=0, CS0=1, DA=111)
#define IDE_REG_ALT_STATUS 0x16    //(CS1=1, CS0=0, DA=110)

// ATAPI register aliases
#define ATAPI_MAGIC_ID 0xEB14
#define ATAPI_REG_FEATURE   0x09	  //(CS1=0, CS0=1, DA=001)
#define ATAPI_REG_BYTECOUNT_LOW  0x0c //(CS1=0, CS0=1, DA=100)
#define ATAPI_REG_BYTECOUNT_HIGH 0x0d //(CS1=0, CS0=1, DA=101)
#define ATAPI_REG_COMMAND 0x0f	      //(CS1=0, CS0=1, DA=111)

// DEVHEAD register
#define IDE_REG_DEVICE_DEV 0x10

// STATUS register
#define IDE_REG_STATUS_BUSY 0x80
#define IDE_REG_STATUS_DRQ  0x08

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
}


// status registr subs
uint8_t ide_busy(uint8_t *status) {
	*status = (uint8_t)ide_read(IDE_REG_STATUS);
	return(*status & IDE_REG_STATUS_BUSY);
}

uint8_t ide_drq(uint8_t status) {
	return(status & IDE_REG_STATUS_DRQ);
}

// wait for not busy
// != 0 - timeout
uint8_t ide_wait_not_busy(uint8_t *status) {
	unsigned int timeout = IDE_TIMEOUT;
	while (ide_busy(status)) {
		if (--timeout == 0) {
			return(1);
		}
		Delay_Ms(1);
	}
	return(0);
}

uint8_t ide_wait_drq() {
	uint8_t status;
	unsigned int timeout = IDE_TIMEOUT;

	do {
		if (ide_wait_not_busy(&status)) {
			return(1);
		}
		if (ide_drq(status)) {
			return(0);
		}
		Delay_Ms(1);
	} while (--timeout);
	return(1);
}

uint8_t ide_wait_not_drq() {
	uint8_t status;
	unsigned int timeout = IDE_TIMEOUT;

	do {
		if (ide_wait_not_busy(&status)) {
			return(1);
		}
		if (!ide_drq(status)) {
			return(0);
		}
		Delay_Ms(1);
	} while (--timeout);
	return(1);
}

void ide_select_device(uint8_t dev) {
	ide_wait_not_drq();
	if (dev) {
		ide_write(IDE_REG_DEVICE, 0xff10);
	} else {
		ide_write(IDE_REG_DEVICE, 0xff00);
	}
}


void ide_init(void) {
	ide_write(IDE_REG_ALT_STATUS, 0xff02); // disable INTRQ
}

// ATAPI
uint8_t is_atapi_device() {
	uint16_t in16;
	in16 = ide_read(IDE_REG_CYLINDER_LOW) & 0xff;
	in16 |= ide_read(IDE_REG_CYLINDER_HIGH) << 8;

	return(in16 == ATAPI_MAGIC_ID);
}

void atapi_read_packet_word(uint16_t *data) {
	if (ide_wait_drq()) {
		return;
	}
	*data = ide_read(IDE_REG_DATA);
}

void atapi_read_packet_word_skip(void) {
	uint16_t dummy;
	atapi_read_packet_word(&dummy);
}

void atapi_read_packet(uint8_t *data, uint16_t count) {
	count >>= 1;
    for(uint16_t i = 0; i < count; ++i) {
		if (ide_wait_drq()) {
			return;
		}
		((uint16_t*)data)[i] = ide_read(IDE_REG_DATA);
    }
}

void atapi_read_packet_skip(uint16_t count) {
	count >>= 1;
    for(uint16_t i = 0; i < count; ++i) {
		if (ide_wait_drq()) {
			return;
		}
		ide_read(IDE_REG_DATA);
    }
}

void atapi_read_packet_string(char *data, uint16_t count) {
	uint16_t len = count;
	count >>= 1;
    for(uint16_t i = 0; i < count; ++i) {
		if (ide_wait_drq()) {
			return;
		}
		((uint16_t*)data)[i] = ide_read(IDE_REG_DATA);
    }
	data[len] = 0;
}

// device info answer
#define ATA_IDENTIFYPACKETDEVICE_SERIALNUMBER_LEN 20
#define ATA_IDENTIFYPACKETDEVICE_FIRMWAREREVISION_LEN 8
#define ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN 40

#define ATA_COMMAND_IDENTIFYPACKETDEVICE 0xa1

typedef struct {
	uint16_t general_config;
	char serial_number[ATA_IDENTIFYPACKETDEVICE_SERIALNUMBER_LEN + 1];
	char firmware_rev[ATA_IDENTIFYPACKETDEVICE_FIRMWAREREVISION_LEN + 1];
	char model[ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN + 1];
	uint8_t capabilities;
	uint8_t pio_mode;
} atapi_device_information_t;

void atapi_identify_packet_device(atapi_device_information_t * info) {
	ide_write(ATAPI_REG_FEATURE, 0xff00);
	ide_write(ATAPI_REG_BYTECOUNT_LOW, 0xff00);
	ide_write(ATAPI_REG_BYTECOUNT_HIGH, 0xff01);
	ide_write(ATAPI_REG_COMMAND, ATA_COMMAND_IDENTIFYPACKETDEVICE);

	uint16_t data = 0;

	atapi_read_packet_word(&info->general_config);			// word 0: general configuration
	/*
	atapi_read_packet_skip(18);         					// words 1 to 9: reserved
	atapi_read_packet_string(info->serial_number,
		ATA_IDENTIFYPACKETDEVICE_SERIALNUMBER_LEN);         // words 10 to 19: serial number
	atapi_read_packet_skip(6);					            // words 20 to 22: reserved
	atapi_read_packet_string(info->firmware_rev,
		ATA_IDENTIFYPACKETDEVICE_FIRMWAREREVISION_LEN);     // words 23 to 26: firmware revision
	atapi_read_packet_string(info->model,
		ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN);          // words 27 to 46: model number
	atapi_read_packet_skip(4);                              // words 47 to 48: reserved
	atapi_read_packet_word(&data);                          // word 49: capabilities
	info->capabilities = data >> 8;
	atapi_read_packet_word_skip();                          // word 50: reserved
	atapi_read_packet_word(&data);                          // word 51: PIO mode + vendor specific
	info->pio_mode = data >> 8;
	atapi_read_packet_word_skip();					        // word 52: reserved
	atapi_read_packet_skip(406);				            // words 53 to 255: too lazy
*/
	//atapi_waitNoDataRequest();
}
#endif // IDE_H
