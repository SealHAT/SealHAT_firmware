/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_init.h"
#include <peripheral_clk_config.h>
#include <utils.h>
#include <hal_init.h>

struct i2c_m_sync_desc I2C_ENV;

struct usart_os_descriptor EDBG_COM;
uint8_t                    EDBG_COM_buffer[EDBG_COM_BUFFER_SIZE];

void I2C_ENV_PORT_init(void)
{

	gpio_set_pin_pull_mode(PA08,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(PA08, PINMUX_PA08D_SERCOM2_PAD0);

	gpio_set_pin_pull_mode(PA09,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(PA09, PINMUX_PA09D_SERCOM2_PAD1);
}

void I2C_ENV_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM2_GCLK_ID_CORE, CONF_GCLK_SERCOM2_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM2_GCLK_ID_SLOW, CONF_GCLK_SERCOM2_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBCMASK_SERCOM2_bit(MCLK);
}

void I2C_ENV_init(void)
{
	I2C_ENV_CLOCK_init();
	i2c_m_sync_init(&I2C_ENV, SERCOM2);
	I2C_ENV_PORT_init();
}

void EDBG_COM_PORT_init(void)
{

	gpio_set_pin_function(EDBG_COM_TX, PINMUX_PA22C_SERCOM3_PAD0);

	gpio_set_pin_function(EDBG_COM_RX, PINMUX_PA23C_SERCOM3_PAD1);
}

void EDBG_COM_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM3_GCLK_ID_CORE, CONF_GCLK_SERCOM3_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM3_GCLK_ID_SLOW, CONF_GCLK_SERCOM3_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBCMASK_SERCOM3_bit(MCLK);
}

void EDBG_COM_init(void)
{
	EDBG_COM_CLOCK_init();
	usart_os_init(&EDBG_COM, SERCOM3, EDBG_COM_buffer, EDBG_COM_BUFFER_SIZE, (void *)NULL);
	usart_os_enable(&EDBG_COM);
	EDBG_COM_PORT_init();
}

void system_init(void)
{
	init_mcu();

	// GPIO on PA16

	// Set pin direction to output
	gpio_set_pin_direction(DEBUG_1, GPIO_DIRECTION_OUT);

	gpio_set_pin_level(DEBUG_1,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	gpio_set_pin_function(DEBUG_1, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PB01

	// Set pin direction to output
	gpio_set_pin_direction(DEBUG_0, GPIO_DIRECTION_OUT);

	gpio_set_pin_level(DEBUG_0,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	gpio_set_pin_function(DEBUG_0, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PB10

	// Set pin direction to output
	gpio_set_pin_direction(LED0, GPIO_DIRECTION_OUT);

	gpio_set_pin_level(LED0,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	gpio_set_pin_function(LED0, GPIO_PIN_FUNCTION_OFF);

	I2C_ENV_init();

	EDBG_COM_init();
}
