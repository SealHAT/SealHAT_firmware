/*
 * seal_IMU.h
 *
 * Created: 30-Apr-18 22:40:17
 *  Author: Ethan
 */
#include "seal_CTRL.h"
#include "lsm303/LSM303AGR.h"

#ifndef SEAL_IMU_H_
#define SEAL_IMU_H_

#define IMU_STACK_SIZE                  (1000 / sizeof(portSTACK_TYPE))
#define IMU_TASK_PRI                    (tskIDLE_PRIORITY + 3)

#define IMU_DATA_SIZE       (25)
typedef struct __attribute__((__packed__)){
    DATA_HEADER_t header;
    AxesRaw_t     data[IMU_DATA_SIZE];
} IMU_MSG_t;

extern TaskHandle_t xIMU_th;        // IMU task handle

/**
 * This function is the ISR callback intended for use with the Accelerometer Data Ready Interrupt
 * It will be triggered on the rising edge of the interrupt.
 */
void AccelerometerDataReadyISR(void);

/**
 * This function is the ISR callback intended for use with the magnetometer Data Ready Interrupt
 * It will be triggered on the rising edge of the interrupt.
 */
void MagnetometerDataReadyISR(void);

/**
 * This function is the ISR callback intended for use with the Accelerometer Motion Detection Interrupt
 * It will be triggered on the rising edge of the interrupt.
 */
void AccelerometerMotionISR(void);

/**
 * Initializes the resources needed for the IMU task.
 *
 * @return system error code. ERR_NONE if successful, or negative if failure (ERR_NO_MEMORY likely).
 */
int32_t IMU_task_init(void);

/**
 * Sets the IMU (accelerometer and magnetometer) to idle/low-power mode
 * @returns ERR_NONE with success, otherwise a system error code
 */
int32_t IMU_task_deinit(void);

/**
 * The IMU task. Only use as a task in RTOS, never call directly.
 */
void IMU_task(void* pvParameters);

#endif /* SEAL_IMU_H_ */