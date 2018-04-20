/**
 * \file
 *
 * \brief I/O I2C related functionality implementation.
 *
 * Copyright (C) 2015-2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
#include <hal_i2c_m_os.h>
#include <utils.h>
#include <utils_assert.h>

/**
 * \brief Driver version
 */
#define I2C_M_OS_DRIVER_VERSION 0x00000001u

/**
 * \brief Callback function for tx complete
 */
static void i2c_m_os_tx_complete(struct _i2c_m_async_device *const i2c_dev)
{
	struct i2c_m_os_desc *i2c = CONTAINER_OF(i2c_dev, struct i2c_m_os_desc, device);

	if (!(i2c_dev->service.msg.flags & I2C_M_BUSY)) {
		sem_up(&i2c->xfer_sem);
	}
}

/**
 * \brief Callback function for rx complete
 */
static void i2c_m_os_rx_complete(struct _i2c_m_async_device *const i2c_dev)
{
	struct i2c_m_os_desc *i2c = CONTAINER_OF(i2c_dev, struct i2c_m_os_desc, device);

	if (!(i2c_dev->service.msg.flags & I2C_M_BUSY)) {
		sem_up(&i2c->xfer_sem);
	}
}

/**
 * \brief Callback function for I2C error
 */
static void i2c_m_os_error(struct _i2c_m_async_device *const i2c_dev, int32_t error)
{
	struct i2c_m_os_desc *i2c = CONTAINER_OF(i2c_dev, struct i2c_m_os_desc, device);

	i2c->error = error;
	sem_up(&i2c->xfer_sem);
}

/**
 * \brief Rtos version of I2C I/O read
 */
static int32_t i2c_m_os_read(struct io_descriptor *const io, uint8_t *buf, const uint16_t n)
{
	struct i2c_m_os_desc *i2c = CONTAINER_OF(io, struct i2c_m_os_desc, io);
	struct _i2c_m_msg     msg;
	int32_t               ret;

	msg.addr   = i2c->slave_addr;
	msg.len    = n;
	msg.flags  = I2C_M_STOP | I2C_M_RD;
	msg.buffer = buf;

	/* start transfer then return */
	i2c->error = 0;
	ret        = _i2c_m_async_transfer(&i2c->device, &msg);

	if (ret != 0) {
		/* error occurred */
		return ret;
	}

	if (sem_down(&i2c->xfer_sem, ~0) < 0) {
		return ERR_TIMEOUT;
	}

	if (i2c->error) {
		return ERR_IO;
	}

	return (int32_t)n;
}

/**
 * \brief Rtos version of I2C I/O write
 */
static int32_t i2c_m_os_write(struct io_descriptor *const io, const uint8_t *buf, const uint16_t n)
{
	struct i2c_m_os_desc *i2c = CONTAINER_OF(io, struct i2c_m_os_desc, io);
	struct _i2c_m_msg     msg;
	int32_t               ret;

	msg.addr   = i2c->slave_addr;
	msg.len    = n;
	msg.flags  = I2C_M_STOP;
	msg.buffer = (uint8_t *)buf;

	/* start transfer then return */
	i2c->error = 0;
	ret        = _i2c_m_async_transfer(&i2c->device, &msg);

	if (ret != 0) {
		/* error occurred */
		return ret;
	}

	if (sem_down(&i2c->xfer_sem, ~0) < 0) {
		return ERR_TIMEOUT;
	}

	if (i2c->error) {
		return ERR_IO;
	}

	return (int32_t)n;
}

/**
 * \brief Rtos version of i2c initialize
 */
int32_t i2c_m_os_init(struct i2c_m_os_desc *const i2c, void *const hw)
{
	ASSERT(i2c);

	/* Init I/O */
	i2c->io.read  = i2c_m_os_read;
	i2c->io.write = i2c_m_os_write;

	/* Init callbacks */
	_i2c_m_async_register_callback(&i2c->device, I2C_M_ASYNC_DEVICE_TX_COMPLETE, (FUNC_PTR)i2c_m_os_tx_complete);
	_i2c_m_async_register_callback(&i2c->device, I2C_M_ASYNC_DEVICE_RX_COMPLETE, (FUNC_PTR)i2c_m_os_rx_complete);
	_i2c_m_async_register_callback(&i2c->device, I2C_M_ASYNC_DEVICE_ERROR, (FUNC_PTR)i2c_m_os_error);

	sem_init(&i2c->xfer_sem, 0);

	return _i2c_m_async_init(&i2c->device, hw);
}
/**
 * \brief Rtos version Deinitialize i2c master interface
 */
int32_t i2c_m_os_deinit(struct i2c_m_os_desc *const i2c)
{
	ASSERT(i2c);
	sem_deinit(&i2c->xfer_sem);

	i2c->io.read  = NULL;
	i2c->io.write = NULL;

	_i2c_m_async_register_callback(&i2c->device, I2C_M_ASYNC_DEVICE_TX_COMPLETE, NULL);
	_i2c_m_async_register_callback(&i2c->device, I2C_M_ASYNC_DEVICE_RX_COMPLETE, NULL);
	_i2c_m_async_register_callback(&i2c->device, I2C_M_ASYNC_DEVICE_ERROR, NULL);

	return _i2c_m_async_deinit(&i2c->device);
}
/**
 * \brief Rtos version of i2c enable
 */
int32_t i2c_m_os_enable(struct i2c_m_os_desc *const i2c)
{
	ASSERT(i2c);

	if (_i2c_m_async_enable(&i2c->device) < 0) {
		return ERR_DENIED;
	}
	_i2c_m_async_set_irq_state(&i2c->device, I2C_M_ASYNC_DEVICE_TX_COMPLETE, true);
	_i2c_m_async_set_irq_state(&i2c->device, I2C_M_ASYNC_DEVICE_RX_COMPLETE, true);
	_i2c_m_async_set_irq_state(&i2c->device, I2C_M_ASYNC_DEVICE_ERROR, true);
	return 0;
}

/**
 * \brief Rtos version of i2c disable
 */
int32_t i2c_m_os_disable(struct i2c_m_os_desc *const i2c)
{
	ASSERT(i2c);

	if (_i2c_m_async_disable(&i2c->device) < 0) {
		return ERR_BUSY;
	}
	_i2c_m_async_set_irq_state(&i2c->device, I2C_M_ASYNC_DEVICE_TX_COMPLETE, false);
	_i2c_m_async_set_irq_state(&i2c->device, I2C_M_ASYNC_DEVICE_RX_COMPLETE, false);
	_i2c_m_async_set_irq_state(&i2c->device, I2C_M_ASYNC_DEVICE_ERROR, false);
	return 0;
}

/**
 * \brief Rtos version of i2c set slave address
 */
int32_t i2c_m_os_set_slaveaddr(struct i2c_m_os_desc *const i2c, int16_t addr, int32_t addr_len)
{
	return i2c->slave_addr = (addr & 0x3ff) | (addr_len & I2C_M_TEN);
}

/**
 * \brief Rtos version of i2c set baudrate
 */
int32_t i2c_m_os_set_baudrate(struct i2c_m_os_desc *const i2c, uint32_t clkrate, uint32_t baudrate)
{
	return _i2c_m_async_set_baudrate(&i2c->device, clkrate, baudrate);
}

/**
 * \brief Rtos version of i2c transfer
 */
int32_t i2c_m_os_transfer(struct i2c_m_os_desc *const i2c, struct _i2c_m_msg *msg, int n)
{
	int     i;
	int32_t ret;

	for (i = 0; i < n; i++) {
		ret = _i2c_m_async_transfer(&i2c->device, &msg[i]);

		if (ret != 0) {
			/* error occurred */
			return ret;
		}

		if (0 != sem_down(&i2c->xfer_sem, ~0)) {
			return ERR_TIMEOUT;
		}
	}

	return n;
}

/**
 * \brief Send stop condition
 */
int32_t i2c_m_os_send_stop(struct i2c_m_os_desc *const i2c)
{
	return _i2c_m_async_send_stop(&i2c->device);
}

/**
 * \brief Retrieve the current driver version
 */
uint32_t i2c_m_os_get_version(void)
{
	return I2C_M_OS_DRIVER_VERSION;
}
