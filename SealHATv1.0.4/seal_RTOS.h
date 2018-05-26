/*
 * seal_RTOS.h
 *
 * Created: 30-Apr-18 23:06:44
 *  Author: Ethan
 */

#ifndef SEAL_RTOS_H_
#define SEAL_RTOS_H_

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "limits.h"
#include "event_groups.h"
#include "stream_buffer.h"

#include "hal_io.h"
#include "hal_rtos.h"

#include "seal_Types.h"
#include "seal_UTIL.h"

// 24-bit system wide event group. NEVER use the numbers directly, they are subject to change. Use the names.
typedef enum {
    // System state alerts
    EVENT_VBUS          = 0x00000001, // indicated the current VBUS level, use USB API to check USB state
    EVENT_LOW_BATTERY   = 0x00000002, // Indicates the battery has reached a critically low level according to settings
    EVENT_LOGTOFLASH    = 0x00000004, // This bit indicates that the system should be logging data to the flash memory
    EVENT_LOGTOUSB      = 0x00000008, // This bit indicates that the device should be streaming data over USB
    EVENT_DEBUG         = 0x00000010, // This bit indicates that the device is in debug mode. this overrides the other modes.
    EVENT_SYS_4         = 0x00000020,
    EVENT_SYS_5         = 0x00000040,
    EVENT_SYS_6         = 0x00000080,
    EVENT_MASK_SYS      = 0x000000FF, // Mask for watching the system flags

    // IMU events. names assume pin 1 of the IMU is in the upper right
    // Consumers of these flags are responsible for clearing them
    EVENT_MOTION_SHIFT  = 8,            // number of bits to shift the LSM303 motion alerts register to match these bits
    EVENT_MASK_IMU      = 0x0000FF00,   // mask for watching the IMU bits
    EVENT_MASK_IMU_X    = 0x00000300,   // mask for isolating the IMU X axis
    EVENT_MASK_IMU_Y    = 0x00000C00,   // mask for isolating the IMU X axis
    EVENT_MASK_IMU_Z    = 0x00003000,   // mask for isolating the IMU X axis
    EVENT_MOTION_XLOW   = 0x00000100,
    EVENT_MOTION_XHI    = 0x00000200,
    EVENT_MOTION_YLOW   = 0x00000400,
    EVENT_MOTION_YHI    = 0x00000800,
    EVENT_MOTION_ZLOW   = 0x00001000,
    EVENT_MOTION_ZHI    = 0x00002000,
    EVENT_IMU_ACTIVE    = 0x00004000,
    EVENT_IMU_IDLE      = 0x00008000,

    EVENT_FLASH_FULL    = 0x00010000,   // mask for determining if external flash is full
    EVENT_UNUSED_8      = 0x00020000,
    EVENT_UNUSED_9      = 0x00040000,
    EVENT_UNUSED_10     = 0x00080000,
    EVENT_UNUSED_11     = 0x00100000,
    EVENT_UNUSED_12     = 0x00200000,
    EVENT_UNUSED_13     = 0x00400000,
    EVENT_UNUSED_14     = 0x00800000,
    EVENT_MASK_ALL      = 0x00FFFFFF    // mask for all bits
} SYSTEM_EVENT_FLAGS_t;

typedef struct __attribute__((__packed__)){
    SENSOR_CONFIGS config_settings;
    uint32_t       current_flash_addr;
    uint8_t        current_flash_chip;
} EEPROM_STORAGE_t;

extern EventGroupHandle_t   xSYSEVENTS_handle;  // event group
extern EEPROM_STORAGE_t     eeprom_data;        //struct containing sensor and SealHAT configurations
 
void vApplicationIdleHook(void);

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);

#endif /* SEAL_RTOS_H_ */