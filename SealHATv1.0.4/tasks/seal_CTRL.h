/*
 * seal_MSG.h
 *
 * Created: 30-Apr-18 22:47:57
 *  Author: Ethan
 */

#ifndef SEAL_MSG_H_
#define SEAL_MSG_H_

#include "seal_RTOS.h"
#include "seal_Types.h"
#include "seal_USB.h"
#include "sealPrint.h"
#include "storage\flash_io.h"
#include "driver_init.h"

#define CTRL_STACK_SIZE                 (900 / sizeof(portSTACK_TYPE))
#define CTRL_TASK_PRI                   (tskIDLE_PRIORITY + 3)

#define DATA_QUEUE_LENGTH               (3000)
#define HOUR_MS                         (10000)

extern TaskHandle_t       xCTRL_th;           // Message accumulator for USB/MEM

/**
 * This function is the ISR callback intended for use with the VBUS interrupt.
 * It will be called on the rising and falling edge of VBUS.
 */
void vbus_detection_cb(void);

/**
 * Initializes the resources needed for the control task.
 * @return system error code. ERR_NONE if successful, or negative if failure (ERR_NO_MEMORY likely).
 */
int32_t CTRL_task_init(void);

/**
 * The control task. Only use as a task in RTOS, never call directly.
 */
void CTRL_task(void* pvParameters);

/**
 *  Updates the hourly timer used in sensor scheduling
 */
void CTRL_timer_update(TimerHandle_t xTimer);

/**
 *  Updates time from GPS, checks and sets active tasks
 */
void vHourlyTimerCallback( TimerHandle_t xTimer );

#endif /* SEAL_MSG_H_ */
