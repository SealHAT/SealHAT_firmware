/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_examples.h"
#include "driver_init.h"
#include "utils.h"

void I2C_GPS_example(void)
{
	struct io_descriptor *I2C_GPS_io;

	i2c_m_sync_get_io_descriptor(&I2C_GPS, &I2C_GPS_io);
	i2c_m_sync_enable(&I2C_GPS);
	i2c_m_sync_set_slaveaddr(&I2C_GPS, 0x12, I2C_M_SEVEN);
	io_write(I2C_GPS_io, (uint8_t *)"Hello World!", 12);
}

void I2C_ENV_example(void)
{
	struct io_descriptor *I2C_ENV_io;

	i2c_m_sync_get_io_descriptor(&I2C_ENV, &I2C_ENV_io);
	i2c_m_sync_enable(&I2C_ENV);
	i2c_m_sync_set_slaveaddr(&I2C_ENV, 0x12, I2C_M_SEVEN);
	io_write(I2C_ENV_io, (uint8_t *)"Hello World!", 12);
}

void I2C_IMU_example(void)
{
	struct io_descriptor *I2C_IMU_io;

	i2c_m_sync_get_io_descriptor(&I2C_IMU, &I2C_IMU_io);
	i2c_m_sync_enable(&I2C_IMU);
	i2c_m_sync_set_slaveaddr(&I2C_IMU, 0x12, I2C_M_SEVEN);
	io_write(I2C_IMU_io, (uint8_t *)"Hello World!", 12);
}

/**
 * Example of using SPI_MEMORY to write "Hello World" using the IO abstraction.
 */
static uint8_t example_SPI_MEMORY[12] = "Hello World!";

void SPI_MEMORY_example(void)
{
	struct io_descriptor *io;
	spi_m_sync_get_io_descriptor(&SPI_MEMORY, &io);

	spi_m_sync_enable(&SPI_MEMORY);
	io_write(io, example_SPI_MEMORY, 12);
}
