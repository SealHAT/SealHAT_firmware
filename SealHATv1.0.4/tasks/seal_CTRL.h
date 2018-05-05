/*
 * seal_MSG.h
 *
 * Created: 30-Apr-18 22:47:57
 *  Author: Ethan
 */
#include "seal_RTOS.h"
#include "seal_USB.h"
#include "sealPrint.h"

#ifndef SEAL_MSG_H_
#define SEAL_MSG_H_

#define MSG_STACK_SIZE                  (750 / sizeof(portSTACK_TYPE))
#define MSG_TASK_PRI                    (tskIDLE_PRIORITY + 1)

typedef enum {
    DEV_ENV  = 0x10,
    DEV_IMU  = 0x20,
    DEV_GPS  = 0x30,
    DEV_MOD  = 0x40,
    DEV_MEM  = 0x50,
    DEV_CPU  = 0x60,
    DEV_MASK = 0xF0
} DEV_ID_t;

typedef enum {
    ERROR_NONE    = 0x00,
    ERROR_TEMP    = 0x01,
    ERROR_LIGHT   = 0x02,
    ERROR_CRC     = 0x03,
    ERROR_I2C     = 0x04,
    ERROR_TIMEOUT = 0x05
} DEV_ERR_CODES_t;

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

typedef struct __attribute__((__packed__)){
    uint16_t srtSym;    // symbol to indicate start of packet
    uint16_t id;	    // Upper four bits is the device ID, lower four are device specific event flags
    uint32_t timestamp; // timestamp. how many bits?
    uint16_t msTime;    // timestampe ms part
    uint16_t size;		// size of data packet to follow. in bytes or samples? (worst case IMU size in bytes would need a uint16 :( )
} DATA_HEADER_t;

extern TaskHandle_t       xCTRL_th;      // Message accumulator for USB/MEM
extern EventGroupHandle_t xCTRL_eg;      // IMU event group
extern SemaphoreHandle_t  USB_mutex;   // Mutex to control access to USB terminal
extern QueueHandle_t      xDATA_q;       // a message Queue for collecting all logged data

void vbus_detection_cb(void);

int32_t byteQ_write(uint8_t* buff, const uint32_t LEN);

int32_t CTRL_task_init(uint32_t qLength);

void CTRL_task(void* pvParameters);
 
#endif /* SEAL_MSG_H_ */ 