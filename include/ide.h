#ifndef IDE_H
#define IDE_H

#include <string.h>

#include "mcp23017-i2c.h"

#define IDE_TIMEOUT 600 // ms

#define IDE_NDIOW 0 // PC0
#define IDE_NDIOR 0 // PD0
#define IDE_DA0   2 // PD2
#define IDE_DA1   3 // PD3
#define IDE_DA2   4 // PD4
#define IDE_CS0   3 // PC3
#define IDE_CS1   4 // PC4

#define IDE_REG_DATA    0x08       //(CS1=0, CS0=1, DA=000)    10
#define IDE_REG_ERROR   0x09	   //(CS1=0, CS0=1, DA=001)    11
#define IDE_REG_COI     0x0a	   //(CS1=0, CS0=1, DA=010)    12
#define IDE_REG_CYLINDER_LOW  0x0c //(CS1=0, CS0=1, DA=100)    14
#define IDE_REG_CYLINDER_HIGH 0x0d //(CS1=0, CS0=1, DA=101)    15
#define IDE_REG_DEVICE  0x0e	   //(CS1=0, CS0=1, DA=110)    16
#define IDE_REG_STATUS  0x0f	   //(CS1=0, CS0=1, DA=111)    17
#define IDE_REG_COMMAND 0x0f	   //(CS1=0, CS0=1, DA=111)    17
#define IDE_REG_ALT_STATUS 0x16    //(CS1=1, CS0=0, DA=110)    0e

// ATAPI register aliases
#define ATAPI_MAGIC_ID 0xEB14
#define ATAPI_REG_FEATURE   0x09	  //(CS1=0, CS0=1, DA=001) 11
#define ATAPI_REG_BYTECOUNT_LOW  0x0c //(CS1=0, CS0=1, DA=100) 14
#define ATAPI_REG_BYTECOUNT_HIGH 0x0d //(CS1=0, CS0=1, DA=101) 15
#define ATAPI_REG_COMMAND 0x0f	      //(CS1=0, CS0=1, DA=111) 17
#define ATAPI_REG_CONTROL 0x16        //(CS1=1, CS0=0, DA=110) 0e

// DEVHEAD register
#define IDE_REG_DEVICE_DEV 0x10

// STATUS register
#define IDE_REG_STATUS_BUSY 0x80
#define IDE_REG_STATUS_DRQ  0x08

// drive params
uint8_t flags = 0;
#define FLAGS_16_BYTE_PACKET 0x1 // PACKET 16 vs 12 bytes

#define ATA_COMMAND_SELFDIAG             0x90
#define ATA_COMMAND_PACKET               0xa0
#define ATA_COMMAND_IDENTIFYPACKETDEVICE 0xa1

typedef struct __attribute__((packed)) {
	uint8_t track; // track#
	uint8_t m, s, f; // minute, second, frame
} msf_t;

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

	// XXX reset GPIO D5
//#define IDE_RESET 6
//	GPIOD->CFGLR &= ~(0xf << (4 * IDE_RESET));
//	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * IDE_RESET);
//	GPIOD->OUTDR = GPIOD->OUTDR | (1 << IDE_RESET);

	// GPIO C3
	GPIOC->CFGLR &= ~(0xf << (4 * IDE_CS0));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * IDE_CS0);

	// GPIO C4
	GPIOC->CFGLR &= ~(0xf << (4 * IDE_CS1));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * IDE_CS1);
}

void ide_reset(void) {
	GPIOC->BSHR = (1 << IDE_CS0) | (1 << IDE_CS1);
	Delay_Ms(40);
	GPIOC->BSHR = (1 << (16 + IDE_CS0)) | (1 << (16 + IDE_CS1));
	Delay_Ms(40);
}

void ide_set_reg_addr(uint8_t reg) {
	uint8_t tmp_reg;
	tmp_reg = GPIOD->OUTDR & ~((1 << IDE_DA0) | (1 << IDE_DA1) | (1 << IDE_DA2));
	tmp_reg |= (reg & 7) << IDE_DA0;
	GPIOD->OUTDR = tmp_reg;
	printf("reg:%x\n\r", reg);
	/*
	if (reg & IDE_CS0) {
		GPIOC->BSHR = (1 << (16 + IDE_CS1)); // CS1->0 then CS0->1
        //Delay_Us(30);
		GPIOC->BSHR = (1 << IDE_CS0);
	} else {
		GPIOC->BSHR = (1 << (16 + IDE_CS0)); // CS0->0 then CS1->1
        //Delay_Us(30);
		GPIOC->BSHR = (1 << IDE_CS1);
	}
	*/
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

	ide_nDIOW_off();
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
	if (dev) {
		ide_write(IDE_REG_DEVICE, 0xfff0);
	} else {
		ide_write(IDE_REG_DEVICE, 0xffe0);
	}
	ide_write(ATAPI_REG_BYTECOUNT_LOW, 0xff00);
	ide_write(ATAPI_REG_BYTECOUNT_HIGH, 0xff02);
}

// 0 - ok
uint8_t ide_selfdiag(void) {
	ide_write(ATAPI_REG_CONTROL, 0xff02); // disable INTRQ
	ide_write(IDE_REG_COMMAND, ATA_COMMAND_SELFDIAG | 0xff00); // start diag
	uint8_t err = ide_read(IDE_REG_ERROR);
	return(err == 0x1 ? 0 : 1);
}

void ide_init(void) {
	ide_write(ATAPI_REG_CONTROL, 0xff02); // disable INTRQ
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
			printf("atapi_read_packet ide_wait_drq timeout\n\r");
			return;
		}
		((uint16_t*)data)[i] = ide_read(IDE_REG_DATA);
    }
}

void atapi_read_packet_skip(uint16_t count) {
	count >>= 1;
    for(uint16_t i = 0; i < count; ++i) {
		if (ide_wait_drq()) {
			printf("atapi_read_packet_skip ide_wait_drq timeout\n\r");
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
			printf("atapi_read_packet_string ide_wait_drq timeout\n\r");
			return;
		}
		uint16_t tmp = ide_read(IDE_REG_DATA);
		data[i * 2 + 1] = tmp & 0xff;
		tmp >>= 8;
		data[i * 2] = tmp;
    }
	data[len] = 0;
}

void atapi_write_packet(uint8_t const *data, uint16_t count) {
	count >>= 1;
	for(uint16_t i = 0; i < count ; ++i) {
		if (ide_wait_drq()) {
			printf("atapi_write_packet_ ide_wait_drq timeout\n\r");
			return;
		} else {
			ide_write(IDE_REG_DATA, ((uint16_t*)data)[i] );
		}
	}
}

// device info answer
#define ATA_IDENTIFYPACKETDEVICE_SERIALNUMBER_LEN 20
#define ATA_IDENTIFYPACKETDEVICE_FIRMWAREREVISION_LEN 8
#define ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN 40


// char model_number[ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN + 1];

void atapi_identify_packet_device(char *model_number) {
	ide_write(ATAPI_REG_FEATURE, 0xff00);
	ide_write(ATAPI_REG_BYTECOUNT_LOW, 0xff00);
	ide_write(ATAPI_REG_BYTECOUNT_HIGH, 0xff02);
	ide_write(ATAPI_REG_COMMAND, ATA_COMMAND_IDENTIFYPACKETDEVICE);

	Delay_Ms(450);

	uint16_t data = 0;

	atapi_read_packet_word(&data);                          // word 0: general configuration
	if ((data & 3) == 1) {
		flags |= FLAGS_16_BYTE_PACKET;
	}
	atapi_read_packet_skip(18);         					// words 1 to 9: reserved
	//atapi_read_packet_string(info->serial_number,
	//	ATA_IDENTIFYPACKETDEVICE_SERIALNUMBER_LEN);         // words 10 to 19: serial number
	atapi_read_packet_skip(ATA_IDENTIFYPACKETDEVICE_SERIALNUMBER_LEN);

	atapi_read_packet_skip(6);					            // words 20 to 22: reserved
	//atapi_read_packet_string(info->firmware_rev,
	//	ATA_IDENTIFYPACKETDEVICE_FIRMWAREREVISION_LEN);     // words 23 to 26: firmware revision
	atapi_read_packet_skip(ATA_IDENTIFYPACKETDEVICE_FIRMWAREREVISION_LEN);

	//atapi_read_packet_string(info->model,
	//	ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN);          // words 27 to 46: model number
	if (model_number) {
		atapi_read_packet_string(model_number,
			ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN);      // words 27 to 46: model number
		// fix model number tail
		char *ptr = model_number + ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN;
		while (--ptr != model_number) {
			if (*ptr != 0x20 && *ptr <= 0x80) {
				break;
			}
			*ptr = 0;
		}
	} else {
		atapi_read_packet_skip(ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN);
	}
	atapi_read_packet_skip(4);                              // words 47 to 48: reserved
	atapi_read_packet_word(&data);                          // word 49: capabilities
	//info->capabilities = data >> 8;
	atapi_read_packet_word_skip();                          // word 50: reserved
	atapi_read_packet_word(&data);                          // word 51: PIO mode + vendor specific
	//info->pio_mode = data >> 8;
	atapi_read_packet_word_skip();					        // word 52: reserved
	atapi_read_packet_skip(406);				            // words 53 to 255:

	ide_wait_not_drq();
}

uint8_t const cmds[]= {
  0x1B,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=0 Open tray
  0x1B,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=16 Close tray
  0x1B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=32 Stop unit
  0x47,0x00,0x00,0x10,0x28,0x05,0x4C,0x1A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=48 Start PLAY
  0x4B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=64 PAUSE play
  0x4B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=80 RESUME play
  0x43,0x02,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=96 Read TOC
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=112 unit ready
  0x5A,0x00,0x01,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=128 mode sense
  0x42,0x02,0x40,0x01,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=144 rd subch.
  0x03,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // idx=160 req. sense
  0x4E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  // idx=176 Stop disk
};

#define CMD_OPEN_TRAY             (0 * 16)
#define CMD_CLOSE_TRAY            (1 * 16)
#define CMD_STOP_UNIT             (2 * 16)
#define CMD_START_PLAY            (3 * 16)
#define CMD_PAUSE_PLAY            (4 * 16)
#define CMD_RESUME_PLAY           (5 * 16)
#define CMD_READ_TOC              (6 * 16)
#define CMD_UNIT_READY            (7 * 16)
#define CMD_MODE_SENSE            (8 * 16)
#define CMD_READ_SUBCH            (9 * 16)
#define CMD_REQ_SENSE            (10 * 16)
#define CMD_STOP_DISK            (11 * 16)

void atapi_send_cmd(uint8_t const *cmd) {
	//len += flags & FLAGS_16_BYTE_PACKET ? 16 : 12;
	ide_write(ATAPI_REG_CONTROL, 0xff02); // disable INTRQ
	ide_write(ATAPI_REG_FEATURE, 0xff00);
	ide_write(ATAPI_REG_COMMAND, ATA_COMMAND_PACKET);
	atapi_write_packet(cmd, flags & FLAGS_16_BYTE_PACKET ? 16 : 12);

	uint8_t status;
	ide_wait_not_busy(&status);
}

void send_rom_cmd(unsigned int cmd) {
	atapi_send_cmd(&cmds[cmd]);
}

// buf has at least 16 bytes
void send_play_cmd(msf_t const *msf_play, msf_t const *msf_end, uint8_t *buf) {
	memcpy(buf, &cmds[CMD_START_PLAY], 16);
	buf[3] = msf_play->m;
	buf[4] = msf_play->s;
	buf[5] = msf_play->f;
	buf[6] = msf_end->m;
	buf[7] = msf_end->s;
	buf[8] = msf_end->f;
	atapi_send_cmd(buf);
}

// 0 - disk is OK
uint8_t get_disk_type() {
	send_rom_cmd(CMD_MODE_SENSE);
	Delay_Ms(10);
	ide_wait_drq();
	ide_read(IDE_REG_DATA);
	uint8_t type = ide_read(IDE_REG_DATA) & 0xff;

	if (type == 0x02 || type == 0x06 || type == 0x12 || type == 0x16 || type == 0x22 || type == 0x26) {
		type = 0;
	}

	uint8_t status;
	do {
		ide_read(IDE_REG_DATA);
		ide_busy(&status);
	} while(ide_drq(status));
	return(type);
}

// next pair commands used to ask and check drive's condtion
void unit_ready(void) {
	send_rom_cmd(CMD_UNIT_READY);
}

uint8_t req_sense(void) {
	send_rom_cmd(CMD_REQ_SENSE);
	Delay_Ms(10);
	ide_wait_drq();

	uint8_t tmp, ret, status;
	uint8_t cnt = 0;
	do {
		tmp = ide_read(IDE_REG_DATA);
		if (cnt == 6) {
			ret = tmp;
		}
		++cnt;
		ide_read(IDE_REG_STATUS);
		ide_read(IDE_REG_ALT_STATUS);
		ide_busy(&status);
	} while (ide_drq(status));
	return(ret);
}

// Table of contents
msf_t start_msf; // first track address
msf_t end_msf;   // last track number but AFTER last track address
void read_TOC(void){
	uint8_t status;
	msf_t curr_track;
	uint8_t first_track;

	uint16_t data = ide_read(IDE_REG_DATA);
	printf("First track#:%d\n\rLast track#:%d\n\r", data & 0xff, data >> 8);
	first_track = data & 0xff;
	end_msf.track = data >> 8;

	do {
		uint16_t data = ide_read(IDE_REG_DATA);
		data = ide_read(IDE_REG_DATA);
		printf("track#:%d, ", data);
		curr_track.track = data & 0xff;

		data = ide_read(IDE_REG_DATA);
		printf("m:%d, ", data >> 8);
		curr_track.m = data >> 8;

		data = ide_read(IDE_REG_DATA);
		printf("s:%d f:%d\n\r", data & 0xff, data >> 8);
		curr_track.s = data & 0xff;
		curr_track.f = data >> 8;

		if (curr_track.track == first_track) {
			start_msf = curr_track;
		}
		if (curr_track.track == 0xAA) { // can't copy over track no
			end_msf.m = curr_track.m;
			end_msf.s = curr_track.s;
			end_msf.f = curr_track.f;
		}

		ide_busy(&status);
	} while (ide_drq(status));

	printf("start_msf:%d %d:%d.%d\n\r", start_msf.track, start_msf.m, start_msf.s, start_msf.f);
	printf("end_msf:%d %d:%d.%d\n\r", end_msf.track, end_msf.m, end_msf.s, end_msf.f);

	/*
        readIDE(DataReg);                      // TOC Data Length not needed, don't care
        readIDE(DataReg);                      // Read first and last session
        s_trck = dataLval;
        e_trck = dataHval;
        do{
           readIDE(DataReg);                   // Skip Session no. ADR and control fields
           readIDE(DataReg);                   // Read curent track number
           c_trck = dataLval;
           readIDE(DataReg);                   // Read M
           c_trck_m = dataHval;                // Store M of curent track
           readIDE(DataReg);                   // Read S and F
           c_trck_s = dataLval;                // Store S of current track
           c_trck_f = dataHval;                // Store F of current track

           if (c_trck == s_trck){              // Store MSF of first track
               fnc[51] = c_trck_m;             //
               fnc[52] = c_trck_s;
               fnc[53] = c_trck_f;
           }
           if (c_trck == a_trck){              // Store MSF of actual track
               d_trck_m = c_trck_m;            //
               d_trck_s = c_trck_s;
               d_trck_f = c_trck_f;
           }
           if (c_trck == 0xAA){                // Store MSF of end position
               fnc[54] = c_trck_m;
               fnc[55] = c_trck_s;
               fnc[56] = c_trck_f;
           }
           readIDE(ComSReg);
        } while(dataLval & (1<<3));            // Read data from DataRegister until DRQ=0
		*/
}

void get_TOC(void) {
	while (1) {
		send_rom_cmd(CMD_READ_TOC);
		Delay_Ms(10);
		ide_wait_drq();

		uint16_t len = ide_read(IDE_REG_DATA); // data length
		if (len) {
			break;
		}
		printf("TOC length:%d\n\r", len);
	}
	read_TOC();
}

#endif // IDE_H
