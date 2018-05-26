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
#include "state_functions.h"

#define CTRL_STACK_SIZE                 (4000 / sizeof(portSTACK_TYPE))
#define CTRL_TASK_PRI                   (tskIDLE_PRIORITY + 1)
//#define DATA_QUEUE_LENGTH               (3000)

extern TaskHandle_t       xCTRL_th;           // Message accumulator for USB/MEM

/**
 * This function is the ISR callback intended for use with the VBUS interrupt.
 * It will be called on the rising and falling edge of VBUS.
 */
void vbus_detection_cb(void);

/**
 * This function fills a header with the current timestamp with seconds and milliseconds.
 * @param header [IN] pointer to a data header struct to fill with the time.
 */
void timestamp_FillHeader(DATA_HEADER_t* header);

/**
 * Function to write to the control
 * !!! NEVER USE FROM AN ISR OR OUTSIDE OF THE RTOS CONTEXT !!!
 * This function will always write the total number of bytes requested or fail
 * the data is COPIED into the buffer. if is safe to use the data again as soon as this
 * function returns success.
 *
 * @param buff [IN] pointer to an object to write to the stream queue
 * @param LEN [IN] the size of the object in bytes
 * @return Positive Value  - the number of bytes written, which will always be the number requested.
 *         ERR_NO_RESOURCE - If there is not enough space the function will return ERR_NO_RESOURCE
 *         ERR_FAILURE     - If the mutex is taken by another task
 */
int32_t ctrlLog_write(uint8_t* buff, const uint32_t LEN);

/**
 * Function to write to the control queue from an ISR
 * This function will always write the total number of bytes requested or fail
 * the data is COPIED into the buffer. if is safe to use the data again as soon as this
 * function returns success.
 *
 * @param buff [IN] pointer to an object to write to the stream queue
 * @param LEN [IN] the size of the object in bytes
 * @return Positive Value  - the number of bytes written, which will always be the number requested.
 *         ERR_NO_RESOURCE - If there is not enough space the function will return ERR_NO_RESOURCE
 *         ERR_FAILURE     - If the mutex is taken by another task
 */
int32_t ctrlLog_writeISR(uint8_t* buff, const uint32_t LEN);

/**
 * Initializes the resources needed for the control task.
 *
 * @return system error code. ERR_NONE if successful, or negative if failure (ERR_NO_MEMORY likely).
 */
int32_t CTRL_task_init(void);

/**
 * The control task. Only use as a task in RTOS, never call directly.
 */
void CTRL_task(void* pvParameters);

#endif /* SEAL_MSG_H_ */ 
