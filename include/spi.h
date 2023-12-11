#ifndef SPI_H
#define SPI_H

/* SPI settings */
void spi_setup(void);

/* Reset gpio and spi */
void spi_init(void);

/* SS */
void spi_nss_low(void);
void spi_nss_high(void);

/* Send one byte */
void spi_send8(uint8_t byte);

/* Wait for spi */
void spi_not_busy_wait(void);

#endif // SPI_H
