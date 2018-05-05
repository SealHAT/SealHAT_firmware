/*
 * seal_IMU.h
 *
 * Created: 30-Apr-18 22:40:17
 *  Author: Ethan
 */
#include "seal_RTOS.h"
#include "seal_CTRL.h"
#include "lsm303/LSM303AGR.h"

#ifndef SEAL_IMU_H_
#define SEAL_IMU_H_

#define IMU_STACK_SIZE                  (1000 / sizeof(portSTACK_TYPE))
#define IMU_TASK_PRI                    (tskIDLE_PRIORITY + 3)

typedef struct __attribute__((__packed__)){
    DATA_HEADER_t header;
    AxesRaw_t     accelData[25];
} IMU_MSG_t;

extern TaskHandle_t xIMU_th;        // IMU

void AccelerometerDataReadyISR(void);

void MagnetometerDataReadyISR(void);

void AccelerometerMotionISR(void);

int32_t IMU_task_init(uint32_t settings);

void IMU_task(void* pvParameters);

#endif /* SEAL_IMU_H_ */