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

// 24-bit system wide event group
typedef enum {
    // System state alerts
    EVENT_VBUS          = 0x00000001, // indicated the current VBUS level, use USB API to check USB state
    EVENT_LOW_BATTERY   = 0x00000002, // Indicates the battery has reached a critically low level according to settings
    EVENT_UNUSED_1      = 0x00000004,
    EVENT_UNUSED_2      = 0x00000008,
    EVENT_MOTION_SURGE  = 0x00000010, // indicates a surge event has been detected
    EVENT_MOTION_SWAY   = 0x00000020, // indicates a sway event has been detected
    EVENT_MOTION_HEAVE  = 0x00000040, // indicates a heave event has been detected
    EVENT_POSITION_1    = 0x00000080,
    EVENT_POSITION_2    = 0x00000100,
    EVENT_POSITION_3    = 0x00000200,
    EVENT_IMU_UNK_1     = 0x00000400,
    EVENT_IMU_UNK_2     = 0x00000800,
    EVENT_UNUSED_3      = 0x00001000,
    EVENT_UNUSED_4      = 0x00002000,
    EVENT_UNUSED_5      = 0x00004000,
    EVENT_UNUSED_6      = 0x00008000,
    EVENT_UNUSED_7      = 0x00010000,
    EVENT_UNUSED_8      = 0x00020000,
    EVENT_UNUSED_9      = 0x00040000,
    EVENT_UNUSED_10     = 0x00080000,
    EVENT_UNUSED_11     = 0x00100000,
    EVENT_UNUSED_12     = 0x00200000,
    EVENT_UNUSED_13     = 0x00400000,
    EVENT_UNUSED_14     = 0x00800000,
    EVENT_MASK_ALL      = 0x00FFFFFF
} SYSTEM_EVENT_FLAGS_t;

extern TaskHandle_t         xCTRL_th;      // Message accumulator for USB/MEM
extern EventGroupHandle_t   xCTRL_eg;      // IMU event group
extern SemaphoreHandle_t    DATA_mutex;    // Mutex to control access to USB terminal
extern StreamBufferHandle_t xDATA_sb;      // stream buffer for getting data into FLASH or USB

void vbus_detection_cb(void);

void timestamp_FillHeader(DATA_HEADER_t* header);

int32_t ctrlLog_write(uint8_t* buff, const uint32_t LEN);

int32_t ctrlLog_writeISR(uint8_t* buff, const uint32_t LEN);

int32_t CTRL_task_init(uint32_t qLength);

void CTRL_task(void* pvParameters);

#endif /* SEAL_MSG_H_ */ 