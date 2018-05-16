/*
 * seal_MSG.h
 *
 * Created: 30-Apr-18 22:47:57
 *  Author: Ethan
 */
#include "seal_RTOS.h"
#include "seal_Types.h"
#include "seal_USB.h"
#include "sealPrint.h"

#ifndef SEAL_MSG_H_
#define SEAL_MSG_H_

#define MSG_STACK_SIZE                  (750 / sizeof(portSTACK_TYPE))
#define MSG_TASK_PRI                    (tskIDLE_PRIORITY + 1)

// 24-bit system wide event group. NEVER use the numbers directly, they are subject to change. Use the names.
typedef enum {
    // System state alerts
    EVENT_VBUS          = 0x00000001, // indicated the current VBUS level, use USB API to check USB state
    EVENT_LOW_BATTERY   = 0x00000002, // Indicates the battery has reached a critically low level according to settings
    EVENT_SYS_1         = 0x00000004,
    EVENT_SYS_2         = 0x00000008,
    EVENT_SYS_3         = 0x00000010,
    EVENT_SYS_4         = 0x00000020,
    EVENT_SYS_5         = 0x00000040,
    EVENT_SYS_6         = 0x00000080,
    EVENT_MASK_SYS      = 0x000000FF, // Mask for watching the system flags

    EVENT_MOTION_SURGE  = 0x00000100, // indicates a surge event has been detected
    EVENT_MOTION_SWAY   = 0x00000200, // indicates a sway event has been detected
    EVENT_MOTION_HEAVE  = 0x00000400, // indicates a heave event has been detected
    EVENT_POSITION_1    = 0x00000800,
    EVENT_POSITION_2    = 0x00001000,
    EVENT_POSITION_3    = 0x00002000,
    EVENT_IMU_UNK_1     = 0x00004000,
    EVENT_IMU_UNK_2     = 0x00008000,
    EVENT_MASK_IMU      = 0x0000FF00, // mask for watching the IMU bits

    EVENT_UNUSED_7      = 0x00010000,
    EVENT_UNUSED_8      = 0x00020000,
    EVENT_UNUSED_9      = 0x00040000,
    EVENT_UNUSED_10     = 0x00080000,
    EVENT_UNUSED_11     = 0x00100000,
    EVENT_UNUSED_12     = 0x00200000,
    EVENT_UNUSED_13     = 0x00400000,
    EVENT_UNUSED_14     = 0x00800000,

    EVENT_MASK_ALL      = 0x00FFFFFF  // mask for all bits
} SYSTEM_EVENT_FLAGS_t;

extern TaskHandle_t         xCTRL_th;      // Message accumulator for USB/MEM
extern EventGroupHandle_t   xCTRL_eg;      // IMU event group
extern SemaphoreHandle_t    DATA_mutex;    // Mutex to control access to USB terminal
extern StreamBufferHandle_t xDATA_sb;      // stream buffer for getting data into FLASH or USB

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
 * @param qLength [IN] the length of the control data queue in bytes. Memory for this queue will be reserved dynamically.
 * @return system error code. ERR_NONE if successful, or negative if failure (ERR_NO_MEMORY likely).
 */
int32_t CTRL_task_init(uint32_t qLength);

/**
 * The control task. Only use as a task in RTOS, never call directly.
 */
void CTRL_task(void* pvParameters);

#endif /* SEAL_MSG_H_ */ 