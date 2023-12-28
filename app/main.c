#include "ch32v003fun.h"

#include <stdio.h>

#include "mcp23017-i2c.h"
#include "adc.h"
#include "ide.h"

uint8_t buf[100];

int main()
{
	SystemInit();

	mcp23017_i2c_setup();
    mcp23017_init(MCP23017_I2C_ADDR);
	ide_setup_pins();
	ide_turn_pins_safe();
	ide_init();
	adc_init();

	uint16_t in16;
	uint8_t status;


	/*
	while(1) {
		ide_reset();
		Delay_Ms(200);
	}
	*/

	ide_reset();
	Delay_Ms(6000);

	ide_select_device(0);
	// self diagnostic
	printf("Self diag:");
	if (ide_selfdiag()) {
		printf("Fail.\n\r");
	} else {
		printf("Good.\n\r");
	}

	// check packet size and get model
	atapi_identify_packet_device((char*)buf);
	printf("Model:%s.\n\r", buf);
	printf("Packet size (bytes):");
	if (flags & FLAGS_16_BYTE_PACKET) {
		printf("16.\n\r");
	} else {
		printf("12.\n\r");
	}

	unit_ready();
	uint8_t sense = req_sense();
	while (sense) {
		Delay_Ms(50);
		unit_ready();
		sense = req_sense();
	}

	int cnt = 4;
	while (--cnt) {
		uint8_t type = get_disk_type();
		if (type != 0) {
			printf("disk type:%x\n\r", type);
			Delay_Ms(3000);
			continue;
		}
		printf("get TOC\n\r");
		get_TOC();
		Delay_Ms(5000);
		break;
	}
	send_play_cmd(&start_msf, &end_msf, buf);
	ide_turn_pins_safe();
	cnt = 35;
	while(1) {
		Delay_Ms(60);
		printf( "cnt:%d adc channels: %4d\n\r", cnt, adc_buffer[0]);
		if (--cnt == 0) {
			ide_reset();
		}
	}
}

