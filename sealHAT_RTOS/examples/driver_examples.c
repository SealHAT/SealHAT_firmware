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

void I2C_ENV_example(void)
{
	struct io_descriptor *I2C_ENV_io;

	i2c_m_sync_get_io_descriptor(&I2C_ENV, &I2C_ENV_io);
	i2c_m_sync_enable(&I2C_ENV);
	i2c_m_sync_set_slaveaddr(&I2C_ENV, 0x12, I2C_M_SEVEN);
	io_write(I2C_ENV_io, (uint8_t *)"Hello World!", 12);
}

/**
 * Example task of using EDBG_COM to echo using the IO abstraction.
 */
void EDBG_COM_example_task(void *p)
{
	struct io_descriptor *io;
	uint16_t              data;

	(void)p;

	usart_os_get_io(&EDBG_COM, &io);

	for (;;) {
		if (io->read(io, (uint8_t *)&data, 1) == 1) {
			io->write(io, (uint8_t *)&data, 1);
		}
	}
}
