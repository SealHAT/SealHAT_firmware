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

static void button_on_PA19_pressed(void)
{
}

static void button_on_PA20_pressed(void)
{
}

static void button_on_PA21_pressed(void)
{
}

static void button_on_PA11_pressed(void)
{
}

static void button_on_PA27_pressed(void)
{
}

/**
 * Example of using EXTERNAL_IRQ
 */
void EXTERNAL_IRQ_example(void)
{

	ext_irq_register(PIN_PA19, button_on_PA19_pressed);
	ext_irq_register(PIN_PA20, button_on_PA20_pressed);
	ext_irq_register(PIN_PA21, button_on_PA21_pressed);
	ext_irq_register(PIN_PA11, button_on_PA11_pressed);
	ext_irq_register(PIN_PA27, button_on_PA27_pressed);
}

/**
 * Example of using RTC_CALENDAR.
 */
static struct calendar_alarm alarm;

static void alarm_cb(struct calendar_descriptor *const descr)
{
	/* alarm expired */
}

void RTC_CALENDAR_example(void)
{
	struct calendar_date date;
	struct calendar_time time;

	calendar_enable(&RTC_CALENDAR);

	date.year  = 2000;
	date.month = 12;
	date.day   = 31;

	time.hour = 12;
	time.min  = 59;
	time.sec  = 59;

	calendar_set_date(&RTC_CALENDAR, &date);
	calendar_set_time(&RTC_CALENDAR, &time);

	alarm.cal_alarm.datetime.time.sec = 4;
	alarm.cal_alarm.option            = CALENDAR_ALARM_MATCH_SEC;
	alarm.cal_alarm.mode              = REPEAT;

	calendar_set_alarm(&RTC_CALENDAR, &alarm, alarm_cb);
}

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
