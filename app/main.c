#include "ch32v003fun.h"

#include <stdio.h>

#include "mcp23017-i2c.h"
#include "ide.h"

int main()
{
	SystemInit();

	mcp23017_i2c_setup();
    mcp23017_init(MCP23017_I2C_ADDR);
	ide_setup_pins();

	uint8_t const all_out[] = {0, 0, 0};
	uint8_t neg_pins[] = {0x12, 0xff, 0xff};

	ide_turn_pins_safe();

	uint8_t input[2];
	uint16_t in16;

	while (1) {
		while (!ide_not_busy());

		ide_write(IDE_REG_DEVICE, 0xe0);
		while (!ide_not_busy());

		in16 = ide_read(IDE_REG_CYLINDER_LOW);
		in16 = ide_read(IDE_REG_CYLINDER_HIGH);
		Delay_Ms(250);
	}
	ide_turn_pins_safe();
}

