#ifndef MCP23017_I2C_H
#define MCP23017_I2C_H

/* Based on 05-07-2023 E. Brombaugh oled i2c */

// MCP23017 I2C address
#define MCP23017_I2C_ADDR 0x20
//#define MCP23017_I2C_ADDR 0x21
//#define MCP23017_I2C_ADDR 0x22
//#define MCP23017_I2C_ADDR 0x23
//#define MCP23017_I2C_ADDR 0x24
//#define MCP23017_I2C_ADDR 0x25
//#define MCP23017_I2C_ADDR 0x26
//#define MCP23017_I2C_ADDR 0x27

// Registers
#define MCP23017_IODIRA 0
#define MCP23017_IODIRB 1
#define MCP23017_IOCON  0xa

// IOCON bit for disable sequential operations
#define MCP23017_IOCON_SEQOP 0x20


#define MCP23017_I2C_FREQ    48 // MHz
#define MCP23017_I2C_DUTY33

#define TIMEOUT_MAX 100000

void mcp23017_i2c_setup(void) {
    // Enable GPIOC and I2C
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

    // PC1 is SDA, 10MHz Output, alt func, open-drain
    GPIOC->CFGLR &= ~(0xf << (4 * 1));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF) << (4 * 1);

    // PC2 is SCL, 10MHz Output, alt func, open-drain
    GPIOC->CFGLR &= ~(0xf << (4 * 2));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF) << (4 * 2);

    // Reset I2C1 to init all regs
    RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
    RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1;

    I2C1->CTLR1 |= I2C_CTLR1_SWRST;
    I2C1->CTLR1 &= ~I2C_CTLR1_SWRST;

    // Set module clock frequency
    I2C1->CTLR2 |= MCP23017_I2C_FREQ & I2C_CTLR2_FREQ;

    // Set clock configuration
	// PCLK/(2* I2Cspeed), 100kHz => PCLK / 200000, 400kHz => PCK / 800000
    I2C1->CKCFGR = (38 & I2C_CKCFGR_CCR) | I2C_CKCFGR_FS; // Fast mode 33% duty cycle

    // Enable I2C
    I2C1->CTLR1 |= I2C_CTLR1_PE;

    // set ACK mode
    I2C1->CTLR1 |= I2C_CTLR1_ACK;
}

/*
 * check for 32-bit event codes
 */
uint8_t mcp23017_i2c_chk_evt(uint32_t event_mask)
{
    /* read order matters here! STAR1 before STAR2!! */
    uint32_t status = I2C1->STAR1 | (I2C1->STAR2 << 16);
    return (status & event_mask) == event_mask;
}

void mcp23017_i2c_error(uint8_t err) {
	static char const *err_str[] = {
		"not busy", "master mode", "transmit mode", "tx empty", "transmit complete",
		"receive mode", "rx ready"
	};

	printf("mcp23017 i2c error - %s timeout\n\r", err_str[err]);
}

uint8_t mcp23017_i2c_send(uint8_t addr, uint8_t const *data, uint8_t sz)
{
    int32_t timeout;

    // wait for not busy
    timeout = TIMEOUT_MAX;
    while ((I2C1->STAR2 & I2C_STAR2_BUSY) && (--timeout));
    if (timeout == -1) {
        mcp23017_i2c_error(0);
		return(1);
	}

    // Set START condition
    I2C1->CTLR1 |= I2C_CTLR1_START;

    // wait for master mode select
    timeout = TIMEOUT_MAX;
    while ((!mcp23017_i2c_chk_evt(I2C_EVENT_MASTER_MODE_SELECT)) && (--timeout));
    if (timeout == -1) {
        mcp23017_i2c_error(1);
		return(1);
	}

    // send 7-bit address + write flag
    I2C1->DATAR = addr << 1;

    // wait for transmit condition
    timeout = TIMEOUT_MAX;
    while ((!mcp23017_i2c_chk_evt(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) && (--timeout));
    if (timeout == -1) {
        mcp23017_i2c_error(2);
		return(1);
	}

    // send data one byte at a time
    while(sz--)
    {
        // wait for TX Empty
        timeout = TIMEOUT_MAX;
        while (!(I2C1->STAR1 & I2C_STAR1_TXE) && (--timeout));
        if (timeout == -1) {
            mcp23017_i2c_error(3);
			return(1);
		}

        // send command
        I2C1->DATAR = *data++;
    }
	    // wait for tx complete
    timeout = TIMEOUT_MAX;
    while ((!mcp23017_i2c_chk_evt(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) && (timeout--));
    if (timeout == -1) {
        mcp23017_i2c_error(4);
		return(1);
	}

    // set STOP condition
    I2C1->CTLR1 |= I2C_CTLR1_STOP;

    return(0);
}

uint8_t mcp23017_i2c_recv(uint8_t addr, uint8_t *data, uint8_t sz)
{
    int32_t timeout;

    // wait for not busy
    timeout = TIMEOUT_MAX;
    while ((I2C1->STAR2 & I2C_STAR2_BUSY) && (--timeout));
    if (timeout == -1) {
        mcp23017_i2c_error(0);
		return(1);
	}

    // Set START condition
    I2C1->CTLR1 |= I2C_CTLR1_START;

    // wait for master mode select
    timeout = TIMEOUT_MAX;
    while ((!mcp23017_i2c_chk_evt(I2C_EVENT_MASTER_MODE_SELECT)) && (--timeout));
    if (timeout == -1) {
        mcp23017_i2c_error(1);
		return(1);
	}

    // send 7-bit address + read flag
    I2C1->DATAR = (addr << 1) + 1;

    // wait for receive condition
    timeout = TIMEOUT_MAX;
    while ((!mcp23017_i2c_chk_evt(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED )) && (--timeout));
    if (timeout == -1) {
        mcp23017_i2c_error(5);
		return(1);
	}

	// receive data one byte at a time
    while(sz--)
    {
        // wait for RX ready
        timeout = TIMEOUT_MAX;
        while ((!mcp23017_i2c_chk_evt(I2C_EVENT_MASTER_BYTE_RECEIVED)) && (--timeout));
        if (timeout == -1) {
            mcp23017_i2c_error(6);
			return(1);
		}

        // read byte
        *data++ = I2C1->DATAR;
    }

    // set STOP condition
    I2C1->CTLR1 |= I2C_CTLR1_STOP;

    return(0);
}

// MCP23017 setup
void mcp23017_init(uint8_t address) {
	static uint8_t const noseq[] = {MCP23017_IOCON, MCP23017_IOCON_SEQOP};
	mcp23017_i2c_send(address, noseq, sizeof(noseq));
}

#endif /* MCP23017_I2C_H */
