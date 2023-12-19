#include "ch32v003fun.h"

#include <stdio.h>

#include "mcp23017-i2c.h"
#include "ide.h"


atapi_device_information_t dev_info;

int main()
{
	SystemInit();

	mcp23017_i2c_setup();
    mcp23017_init(MCP23017_I2C_ADDR);
	ide_setup_pins();
	ide_turn_pins_safe();
	ide_init();

	uint16_t in16;
	uint8_t status;

	while (1) {
		ide_select_device(0);

		printf("Is ATAPI device:%d\n\r", is_atapi_device());
		atapi_identify_packet_device(&dev_info);
		printf("Model:%s\n\r", dev_info.model);
		Delay_Ms(150);
	}
	ide_turn_pins_safe();
}

