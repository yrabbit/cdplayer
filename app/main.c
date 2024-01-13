#include "ch32v003fun.h"

#include <stdio.h>

#define DEBUG

#include "mcp23017-i2c.h"
#include "adc.h"
#include "host-xface.h"
#include "ide.h"

char const version[] = "CD-PLAYER V1.0";
char model[ATA_IDENTIFYPACKETDEVICE_MODELNUMBER_LEN + 1];

uint8_t cmd_buf[16];

#define WAIT_FOR_CMD              0
#define WAIT_FOR_TRACK_N          1
#define WAIT_FOR_PLAY_START_TRACK 2
#define WAIT_FOR_PLAY_STOP_TRACK  3

uint8_t state = WAIT_FOR_CMD;
uint8_t start_track = 0;

int main()
{
#ifdef DEBUG
	int nop_cnt = 0;
#endif
	SystemInit();

	host_xface_init();
	mcp23017_i2c_setup();
    mcp23017_init(MCP23017_I2C_ADDR);
	ide_setup_pins();
	ide_turn_pins_safe();
	ide_init();
	adc_init();

	ide_reset();
	Delay_Ms(6000);

	ide_select_device(0);
	// self diagnostic
#ifdef DEBUG
	printf("Self diag:");
#endif
	if (ide_selfdiag()) {
#ifdef DEBUG
		printf("Fail.\n\r");
#endif
	} else {
#ifdef DEBUG
		printf("Good.\n\r");
#endif
	}

	// check packet size and get model
	atapi_identify_packet_device(model);
#ifdef DEBUG
	printf("Model:%s.\n\r", model);
	printf("Packet size (bytes):");
	if (flags & FLAGS_16_BYTE_PACKET) {
		printf("16.\n\r");
	} else {
		printf("12.\n\r");
	}
#endif

	unit_ready();
	uint8_t sense = req_sense();
	while (sense) {
		Delay_Ms(50);
		unit_ready();
		sense = req_sense();
		#ifdef DEBUG
		printf("sense:%x\n\r", sense);
		#endif
		if (sense == 0x3a) {
			break;
		}
	}

	int cnt = 4;
	while (--cnt) {
		uint8_t type = get_disk_type();
		if (type != 0) {
			#ifdef DEBUG
			printf("disk type:%x\n\r", type);
			Delay_Ms(2000);
			#endif
			continue;
		}
		get_TOC();
		break;
	}
#ifdef DEBUG
	printf("===========\n\r");
#endif
	while (1) {
#ifdef DEBUG0
		if (!queue_empty(&xfc_data.in)) {
			if (xfc_data.in.storage[xfc_data.in.read & 0x7f]) {
				printf("in [%x] read:%lu, write:%lu\n\r", xfc_data.in.storage[xfc_data.in.read & 0x7f], xfc_data.in.read, xfc_data.in.write);
			}
		}
		if (!queue_empty(&xfc_data.out)) {
			printf("out [%x] read:%lu, write:%lu\n\r", xfc_data.out.storage[xfc_data.out.read & 0x7f], xfc_data.out.read, xfc_data.out.write);
		}
#endif
		// FSM
		if (!queue_empty(&xfc_data.in)) {
			switch (state) {
				case WAIT_FOR_CMD: {
					uint8_t cmd = read_byte_from_queue(&xfc_data.in);
					switch (cmd) {
						case PROTO_CMD_RESET: {
								#ifdef DEBUG
								printf("RESET\n\r");
								#endif
								uint8_t len = strlen(version);
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(len));
								for (int i = 0; i < len; ++i) {
									write_byte_to_queue(&xfc_data.out, version[i]);
								}
							}
							break;
						case PROTO_CMD_GET_MODEL: {
								#ifdef DEBUG
								printf("GET MODEL\n\r");
								#endif
								uint8_t len = strlen(model);
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(len));
								for (int i = 0; i < len; ++i) {
									write_byte_to_queue(&xfc_data.out, model[i]);
								}
							}
							break;
						case PROTO_CMD_EJECT: {
								uint8_t open_load = PROTO_EJECT_OPEN;
								if (get_disk_type() == 0x71) {
									open_load = PROTO_EJECT_LOAD;
								}
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(1));
								write_byte_to_queue(&xfc_data.out, open_load);
								if (open_load == PROTO_EJECT_OPEN) {
									#ifdef DEBUG
									printf("EJECT eject\n\r");
									#endif
									eject_disk();
								} else {
									#ifdef DEBUG
									printf("EJECT load\n\r");
									#endif
									load_disk();
								}
							}
							break;
						case PROTO_CMD_GET_DISK: {
								#ifdef DEBUG
								printf("GET DISK\n\r");
								#endif
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
								#ifdef DEBUG
								printf("GET TRACK CNT\n\r");
								#endif
								if (!toc_len) {
									get_TOC();
								}
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(1));
								write_byte_to_queue(&xfc_data.out, toc_len);
							}
							break;
						case PROTO_CMD_GET_STATUS: {
								#ifdef DEBUG
								printf("GET STATUS\n\r");
								#endif
								uint8_t status = get_subch();
								#ifdef DEBUG
								printf("status:%d\n\r", status);
								#endif
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(5));
								write_byte_to_queue(&xfc_data.out, status);
								write_byte_to_queue(&xfc_data.out, current_track.track);
								write_byte_to_queue(&xfc_data.out, current_track.m);
								write_byte_to_queue(&xfc_data.out, current_track.s);
								write_byte_to_queue(&xfc_data.out, current_track.f);
							}
							break;
						case PROTO_CMD_GET_TRACK_INFO: {
								state = WAIT_FOR_TRACK_N;
								if (!toc_len) {
									get_TOC();
								}
							}
							break;
						case PROTO_CMD_PLAY_TRACK: {
								state = WAIT_FOR_PLAY_START_TRACK;
							}
							break;
						case PROTO_CMD_PAUSE_PLAY: {
								#ifdef DEBUG
								printf("PAUSE PLAY\n\r");
								#endif
								pause_play();
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(0));
							}
							break;
						case PROTO_CMD_RESUME_PLAY: {
								#ifdef DEBUG
								printf("RESUME PLAY\n\r");
								#endif
								resume_play();
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(0));
							}
							break;
						case PROTO_CMD_STOP_PLAY: {
								#ifdef DEBUG
								printf("STOP PLAY\n\r");
								#endif
								stop_play();
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(0));
							}
							break;
						case PROTO_CMD_NEXT: {
								#ifdef DEBUG
								printf("NEXT\n\r");
								#endif
								uint8_t track = find_next_track();
								if (track != 0xff) {
									send_play_cmd(&toc[track], &toc[toc_len - 1], cmd_buf);
								}
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(0));
							}
							break;
						case PROTO_CMD_PREV: {
								#ifdef DEBUG
								printf("PREV\n\r");
								#endif
								uint8_t track = find_prev_track();
								if (track != 0xff) {
									send_play_cmd(&toc[track], &toc[toc_len - 1], cmd_buf);
								}
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(0));
							}
							break;
						case PROTO_CMD_GET_LEVEL: {
								#ifdef DEBUG
								printf("GET LEVEL\n\r");
								#endif
								write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(2));
								write_byte_to_queue(&xfc_data.out, adc_buffer[0]);
								write_byte_to_queue(&xfc_data.out, adc_buffer[1]);
							}
							break;
						default:
							#ifdef DEBUG
							//printf("cmd:%x\n\r", cmd);
							if (++nop_cnt == 1000) {
								nop_cnt = 0;
								printf("NOP\n\r");
							}
							#endif
							break;
					}
				}
				break;
				case WAIT_FOR_TRACK_N: {
						state = WAIT_FOR_CMD;
						uint8_t track = read_byte_from_queue(&xfc_data.in);
						#ifdef DEBUG
						//printf("GET TRACK #%d DESC\n\r", track);
						#endif
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
				case WAIT_FOR_PLAY_START_TRACK: {
						state = WAIT_FOR_PLAY_STOP_TRACK;
						start_track = read_byte_from_queue(&xfc_data.in);
						#ifdef DEBUG
						printf("PLAY TRACK START#%d\n\r", start_track);
						#endif
						if (start_track >= toc_len) {
							start_track = toc_len - 1;
						}
					}
					break;
				case WAIT_FOR_PLAY_STOP_TRACK: {
						state = WAIT_FOR_CMD;
						uint8_t track = read_byte_from_queue(&xfc_data.in);
						#ifdef DEBUG
						printf("PLAY TRACK STOP#%d\n\r", track);
						#endif
						if (track >= toc_len) {
							track = toc_len - 1;
						}
						write_byte_to_queue(&xfc_data.out, MAKE_ANSWER(0));
						send_play_cmd(&toc[start_track], &toc[track], cmd_buf);
					}
					break;
				default:
					#ifdef DEBUG
					printf("UNK state:%d\n\r", state);
					#endif
					break;
			}
		}
	}
}

