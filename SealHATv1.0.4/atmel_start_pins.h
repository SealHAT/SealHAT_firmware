/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */
#ifndef ATMEL_START_PINS_H_INCLUDED
#define ATMEL_START_PINS_H_INCLUDED

#include <hal_gpio.h>

// SAML21 has 9 pin functions

#define GPIO_PIN_FUNCTION_A 0
#define GPIO_PIN_FUNCTION_B 1
#define GPIO_PIN_FUNCTION_C 2
#define GPIO_PIN_FUNCTION_D 3
#define GPIO_PIN_FUNCTION_E 4
#define GPIO_PIN_FUNCTION_F 5
#define GPIO_PIN_FUNCTION_G 6
#define GPIO_PIN_FUNCTION_H 7
#define GPIO_PIN_FUNCTION_I 8

#define BATTERY_LEVEL GPIO(GPIO_PORTA, 2)
#define PA08 GPIO(GPIO_PORTA, 8)
#define PA09 GPIO(GPIO_PORTA, 9)
#define ENV_IRQ GPIO(GPIO_PORTA, 10)
#define VBUS_DETECT GPIO(GPIO_PORTA, 11)
#define PA12 GPIO(GPIO_PORTA, 12)
#define PA13 GPIO(GPIO_PORTA, 13)
#define MEM_CS2 GPIO(GPIO_PORTA, 14)
#define PA15 GPIO(GPIO_PORTA, 15)
#define PA16 GPIO(GPIO_PORTA, 16)
#define PA17 GPIO(GPIO_PORTA, 17)
#define GPS_TIMEPULSE GPIO(GPIO_PORTA, 18)
#define GPS_TXD GPIO(GPIO_PORTA, 19)
#define IMU_INT1_XL GPIO(GPIO_PORTA, 20)
#define IMU_INT2_XL GPIO(GPIO_PORTA, 21)
#define PA22 GPIO(GPIO_PORTA, 22)
#define PA23 GPIO(GPIO_PORTA, 23)
#define PA24 GPIO(GPIO_PORTA, 24)
#define PA25 GPIO(GPIO_PORTA, 25)
#define IMU_INT_MAG GPIO(GPIO_PORTA, 27)
#define LED_GREEN GPIO(GPIO_PORTB, 2)
#define LED_RED GPIO(GPIO_PORTB, 3)
#define MEM_CS0 GPIO(GPIO_PORTB, 10)
#define MEM_CS1 GPIO(GPIO_PORTB, 11)
#define GPS_EXT_INT GPIO(GPIO_PORTB, 22)
#define GPS_RESET GPIO(GPIO_PORTB, 23)

#endif // ATMEL_START_PINS_H_INCLUDED
