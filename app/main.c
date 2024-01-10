#include "ch32v003fun.h"

#include <stdio.h>

#include "mcp23017-i2c.h"
#include "adc.h"
#include "host-xface.h"
#include "ide.h"

char const version[] = "CD-PLAYER V1.0";
char model[ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN + 1];

uint8_t buf[100];

#define WAIT_FOR_CMD     0
#define WAIT_FOR_TRACK_N 1
uint8_t state = WAIT_FOR_CMD;

int main()
{
	SystemInit();

	mcp23017_i2c_setup();
    mcp23017_init(MCP23017_I2C_ADDR);
	ide_setup_pins();
	ide_turn_pins_safe();
	ide_init();
	adc_init();
	host_xface_init();

	uint16_t in16;
	uint8_t status;

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
	atapi_identify_packet_device(model);
	printf("Model:%s.\n\r", model);
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
		break;
	}
	get_TOC();
	/*
	send_play_cmd(&start_msf, &end_msf, buf);
	ide_turn_pins_safe();
	cnt = 100;
	while(1) {
		Delay_Ms(60);
		printf( "cnt:%d, adc channels: %4d %4d\n\r", cnt, adc_buffer[0], adc_buffer[1]);
		if (--cnt == 0) {
			ide_reset();
		}
	}
	*/
	printf("===========\n\r");
	while (1) {
		if (!queue_empty(&xfc_data.in)) {
			printf("in [%x] read:%lu, write:%lu\n\r", xfc_data.in.storage[xfc_data.in.read & 0x7f], xfc_data.in.read, xfc_data.in.write);
		}
		if (!queue_empty(&xfc_data.out)) {
			printf("out [%x] read:%lu, write:%lu\n\r", xfc_data.out.storage[xfc_data.out.read & 0x7f], xfc_data.out.read, xfc_data.out.write);
		}
		// FSM
		if (!queue_empty(&xfc_data.in)) {
			switch (state) {
				case WAIT_FOR_CMD: {
					uint8_t cmd = read_byte_from_queue(&xfc_data.in);
					switch (cmd) {
						case PROTO_CMD_RESET: {
								printf("RESET\n\r");
								uint8_t len = strlen(version);
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(len));
								for (int i = 0; i < len; ++i) {
									write_byte_to_queue(&xfc_data.out, version[i]);
								}
							}
							break;
						case PROTO_CMD_GET_MODEL: {
								printf("GET MODEL\n\r");
								uint8_t len = strlen(model);
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(len));
								for (int i = 0; i < len; ++i) {
									write_byte_to_queue(&xfc_data.out, model[i]);
								}
							}
							break;
						case PROTO_CMD_EJECT: {
								printf("EJECT\n\r");
								uint8_t open_load = PROTO_EJECT_OPEN;
								if (get_disk_type() == 0x71) {
									open_load = PROTO_EJECT_LOAD;
								}
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(1));
								write_byte_to_queue(&xfc_data.out, open_load);
								if (open_load == PROTO_EJECT_OPEN) {
									eject_disk();
								} else {
									load_disk();
								}
							}
							break;
						case PROTO_CMD_GET_DISK: {
								printf("GET DISK\n\r");
								uint8_t type;
								switch (get_disk_type()) {
									case 0x00:
										type = PROTO_GET_DISK_AUDIO;
										break;
									case 0x71:
										type = PROTO_GET_DISK_OPEN;
										break;
									default:
										type = PROTO_GET_DISK_UNK;
										break;
								}
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(1));
								write_byte_to_queue(&xfc_data.out, type);
							}
							break;
						case PROTO_CMD_GET_TRACK_CNT: {
								printf("GET TRACK CNT\n\r");
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(1));
								write_byte_to_queue(&xfc_data.out, toc_len);
							}
							break;
						case PROTO_CMD_GET_TRACK_DESC: {
								state = WAIT_FOR_TRACK_N;
							}
							break;
						default:
							printf("cmd:%x\n\r", cmd);
							break;
					}
				}
				break;
				case WAIT_FOR_TRACK_N: {
						state = WAIT_FOR_CMD;
						uint8_t track = read_byte_from_queue(&xfc_data.in);
						printf("GET TRACK #%d DESC\n\r", track);
						if (track >= toc_len) {
							track = toc_len - 1;
						}
						write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(4));
						write_byte_to_queue(&xfc_data.out, toc[track].track);
						write_byte_to_queue(&xfc_data.out, toc[track].m);
						write_byte_to_queue(&xfc_data.out, toc[track].s);
						write_byte_to_queue(&xfc_data.out, toc[track].f);
					}
					break;
				default:
					printf("UNK state:%d\n\r", state);
					break;
			}
		}
	}
}

