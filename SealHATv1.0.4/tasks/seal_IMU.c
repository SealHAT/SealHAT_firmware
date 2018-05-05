/*
 * seal_IMU.c
 *
 * Created: 30-Apr-18 22:53:26
 *  Author: Ethan
 */

#include "seal_IMU.h"

#define ACC_DATA_READY      (0x01)
#define MAG_DATA_READY      (0x02)
#define MOTION_DETECT       (0x04)

TaskHandle_t       xIMU_th;     // IMU task handle

void AccelerometerDataReadyISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // will be set to true by notify if we are awakening a higher priority task

    /* Notify the IMU task that the ACCEL FIFO is ready to read */
    xTaskNotifyFromISR(xIMU_th, ACC_DATA_READY, eSetBits, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void MagnetometerDataReadyISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // will be set to true by notify if we are awakening a higher priority task

    gpio_set_pin_level(MOD9, true);
    /* Notify the IMU task that the ACCEL FIFO is ready to read */
    xTaskNotifyFromISR(xIMU_th, MAG_DATA_READY, eSetBits, &xHigherPriorityTaskWoken);

    gpio_set_pin_level(MOD9, false);
    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task.*/
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void AccelerometerMotionISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // will be set to true by notify if we are awakening a higher priority task

    /* Notify the IMU task that the ACCEL FIFO is ready to read */
    xTaskNotifyFromISR(xIMU_th, MOTION_DETECT, eSetBits, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task.*/
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

int32_t IMU_task_init(uint32_t settings)
{
    return ( xTaskCreate(IMU_task, "IMU", IMU_STACK_SIZE, (void*)settings, IMU_TASK_PRI, &xIMU_th) == pdPASS ? ERR_NONE : ERR_NO_MEMORY);
}

void IMU_task(void* pvParameters)
{
//    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 3000 );     // max block time, set to slightly more than accelerometer ISR period
    BaseType_t  xResult;                // holds return value of blocking function
    int32_t     err = 0;                // for catching API errors
    uint32_t    ulNotifyValue;          // notification value from ISRs
    IMU_MSG_t   msg;                    // reads the data
    AxesRaw_t   magData;                // hold magnetometer data
    (void)pvParameters;

    // initialize the temperature sensor
    lsm303_init(&I2C_IMU);
    lsm303_acc_startFIFO(ACC_SCALE_2G, ACC_HR_50_HZ);
    lsm303_mag_start(MAG_LP_50_HZ);

    // initialize the message header
    msg.header.srtSym    = 0xDEAD;
    msg.header.id        = DEV_IMU;
    msg.header.size      = sizeof(AxesRaw_t)*25;
    msg.header.timestamp = 0;

    // enable the data ready interrupts
    ext_irq_register(IMU_INT1_XL, AccelerometerDataReadyISR);
    ext_irq_register(IMU_INT2_XL, AccelerometerMotionISR);
    ext_irq_register(IMU_INT_MAG, MagnetometerDataReadyISR);

//    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for(;;) {

        xResult = xTaskNotifyWait( pdFALSE,          /* Don't clear bits on entry. */
                                   ULONG_MAX,        /* Clear all bits on exit. */
                                   &ulNotifyValue,   /* Stores the notified value. */
                                   xMaxBlockTime );

        if( pdPASS == xResult ) {

            if( ACC_DATA_READY & ulNotifyValue ) {
                portENTER_CRITICAL();
                err = lsm303_acc_FIFOread(&msg.accelData[0], 25, NULL);
                portEXIT_CRITICAL();
                msg.header.timestamp++;

                byteQ_write((uint8_t*)&msg, sizeof(IMU_MSG_t));
            }

            if( MAG_DATA_READY & ulNotifyValue ) {
                portENTER_CRITICAL();
                gpio_set_pin_level(MOD9, true);
                err = lsm303_mag_rawRead(&magData);
                gpio_set_pin_level(MOD9, false);
                portEXIT_CRITICAL();
            }

            if( MOTION_DETECT & ulNotifyValue ) {
                // TODO - read type of motion detected and trigger callback
            }
        }
        else {
            DATA_HEADER_t tmOut;
            tmOut.id        = DEV_IMU | ERROR_TIMEOUT;
            tmOut.size      = 0;
            tmOut.timestamp = 65;

            byteQ_write((uint8_t*)&tmOut, sizeof(DATA_HEADER_t));
        }
    } // END FOREVER LOOP
}