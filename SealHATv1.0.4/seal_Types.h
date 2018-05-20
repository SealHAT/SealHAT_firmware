/*
 * seal_Types.h
 *
 * Created: 15-May-18 22:45:12
 * Author: Ethan
 */

#ifndef SEAL_TYPES_H_
#define SEAL_TYPES_H_

#define SEAL_HAT_VERSION        (1.0)

#define MSG_START_SYM           (0xADDE)

/** Sensor types */
typedef enum {
    DEVICE_ID_RESERVED          = 0x00,
    DEVICE_ID_ENVIRONMENTAL     = 0x10,
    DEVICE_ID_LIGHT             = 0x20,
    DEVICE_ID_TEMPERATURE       = 0x30,
    DEVICE_ID_ACCELEROMETER     = 0x40,
    DEVICE_ID_MAGNETIC_FIELD    = 0x50,
    DEVICE_ID_GYROSCOPE         = 0x60,
    DEVICE_ID_PRESSURE          = 0x70,
    DEVICE_ID_DEPTH             = 0x80,
    DEVICE_ID_GPS               = 0x90,
    DEVICE_ID_EKG               = 0xA0,
    DEVICE_ID_UNUSED            = 0xB0,
    DEVICE_ID_UNUSED1           = 0xC0,
    DEVICE_ID_UNUSED2           = 0xD0,
    DEVICE_ID_SYSTEM            = 0xE0,
    DEVICE_ID_MASK              = 0xF0
} DEVICE_ID_t;

/** Device error codes and flags **/
typedef enum {
    DEVICE_ERR_NONE             = 0x00,
    DEVICE_ERR_COMMUNICATIONS   = 0x01,
    DEVICE_ERR_TIMEOUT          = 0x02,
    DEVICE_ERR_OVERFLOW         = 0x03,
    DEVICE_ERR_CRC              = 0x04,
    DEVICE_ERR_UNK2             = 0x05,
    DEVICE_ERR_UNK3             = 0x06,
    DEVICE_ERR_UNK4             = 0x07,
    DEVICE_ERR_UNK5             = 0x08,
    DEVICE_ERR_UNK6             = 0x09,
    DEVICE_ERR_UNK7             = 0x0A,
    DEVICE_ERR_UNK8             = 0x0B,
    DEVICE_ERR_UNK9             = 0x0C,
    DEVICE_ERR_UNK10            = 0x0D,
    DEVICE_ERR_UNK11            = 0x0E,
    DEVICE_ERR_MASK             = 0x0F
} DEVICE_ERR_CODES_t;

/** Header for data packets from the device **/
typedef struct __attribute__((__packed__)){
    uint16_t startSym;    // symbol to indicate start of packet
    uint16_t id;	    // Upper four bits is the device ID, lower four are device specific event flags
    uint32_t timestamp; // timestamp. how many bits?
    uint16_t msTime;    // timestamp ms part
    uint16_t size;		// size of data packet to follow in bytes
} DATA_HEADER_t;

#endif /* SEAL_TYPES_H_ */