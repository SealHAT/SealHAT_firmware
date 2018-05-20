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

    /* Notify the IMU task that the ACCEL FIFO is ready to read */
    xTaskNotifyFromISR(xIMU_th, MAG_DATA_READY, eSetBits, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task.*/
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void AccelerometerMotionISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // will be set to true by notify if we are awakening a higher priority task

    /* Notify the IMU task that there is a motion interrupt */
    xTaskNotifyFromISR(xIMU_th, MOTION_DETECT, eSetBits, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task.*/
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

int32_t IMU_task_init(void)
{
    return ( xTaskCreate(IMU_task, "IMU", IMU_STACK_SIZE, (void*)NULL, IMU_TASK_PRI, &xIMU_th) == pdPASS ? ERR_NONE : ERR_NO_MEMORY);
}

int32_t IMU_task_deinit(void)
{
    int32_t err;

    err = lsm303_acc_toggle();
    err = lsm303_mag_toggle();
    //i2c_m_sync_disable();

    return err;
}

void IMU_task(void* pvParameters)
{
//    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 3500 );     // max block time, set to slightly more than accelerometer ISR period
    BaseType_t  xResult;                // holds return value of blocking function
    int32_t     err = 0;                // for catching API errors
    uint32_t    ulNotifyValue;          // notification value from ISRs
    IMU_MSG_t   accMsg;                 // data Packet for the accelerometer
    IMU_MSG_t   magMsg;                 // data Packet for the magnetometer
    (void)pvParameters;

    // initialize the IMU
    err = lsm303_init(&I2C_IMU);
    err = lsm303_acc_startFIFO(ACC_SCALE_2G, ACC_HR_50_HZ);
    //err = lsm303_mag_start(MAG_LP_50_HZ);
    //lsm303_acc_motionDetectStart(0x03, 800, 1);

    // enable the data ready interrupts
    ext_irq_register(IMU_INT1_XL, AccelerometerDataReadyISR);
    ext_irq_register(IMU_INT_MAG, MagnetometerDataReadyISR);
    ext_irq_register(IMU_INT2_XL, AccelerometerMotionISR);

    // initialize the message headers
    accMsg.header.startSym = MSG_START_SYM;
    accMsg.header.id       = DEVICE_ID_ACCELEROMETER;
    magMsg.header.startSym = MSG_START_SYM;
    magMsg.header.id       = DEVICE_ID_MAGNETIC_FIELD;
    magMsg.header.size     = sizeof(AxesRaw_t)*25;

//    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    for(;;) {

        xResult = xTaskNotifyWait( pdFALSE,          /* Don't clear bits on entry. */
                                   ULONG_MAX,        /* Clear all bits on exit. */
                                   &ulNotifyValue,   /* Stores the notified value. */
                                   xMaxBlockTime );  /* Max time to block before writing an error packet */

        if( pdPASS == xResult ) {
            if( ACC_DATA_READY & ulNotifyValue ) {
                bool overrun;

                portENTER_CRITICAL();
                err = lsm303_acc_FIFOread(&accMsg.data[0], IMU_DATA_SIZE, &overrun);
                portEXIT_CRITICAL();

                // set timestamp and set error flag if needed
                timestamp_FillHeader(&accMsg.header);
                if(err < 0) {
                    accMsg.header.id  |= DEVICE_ERR_COMMUNICATIONS;
                    accMsg.header.size = 0;
                    ctrlLog_write((uint8_t*)&accMsg, sizeof(DATA_HEADER_t));
                    accMsg.header.id &= ~(DEVICE_ERR_MASK);
                }
                else {
                    // TODO: make buffer slightly larger and have the log write calculate size from err and header size.
                    accMsg.header.size = err;   // the number of bytes read on last read
                    ctrlLog_write((uint8_t*)&accMsg, sizeof(IMU_MSG_t));
                }
            } // end of accelerometer state

            if( MAG_DATA_READY & ulNotifyValue ) {
                static uint_fast8_t magItr = 0;

                portENTER_CRITICAL();
                err = lsm303_mag_rawRead(&magMsg.data[magItr]);
                portEXIT_CRITICAL();
                if(err != ERR_NONE) {
                    magMsg.header.id |= DEVICE_ERR_COMMUNICATIONS;
                    magMsg.data[magItr].xAxis = 0xFF;
                    magMsg.data[magItr].yAxis = 0xFF;
                    magMsg.data[magItr].zAxis = 0xFF;
                }
                magItr++;

                if(magItr >= IMU_DATA_SIZE) {
                    timestamp_FillHeader(&magMsg.header);
                    ctrlLog_write((uint8_t*)&magMsg, sizeof(IMU_MSG_t));
                    magMsg.header.id &= ~(DEVICE_ERR_MASK);
                    magItr = 0;
                }
            }

            if( MOTION_DETECT & ulNotifyValue ) {
                uint8_t detect;
                err = lsm303_acc_motionDetectRead(&detect);

                if(!err) {
                    xEventGroupSetBits(xCTRL_eg, ((detect & MOTION_INT_MASK) << EVENT_MOTION_SHIFT));
                }
            }
        }
        else {
            accMsg.header.id  |= DEVICE_ERR_TIMEOUT;
            accMsg.header.size = 0;
            timestamp_FillHeader(&accMsg.header);
            err = ctrlLog_write((uint8_t*)&accMsg, sizeof(DATA_HEADER_t));
            accMsg.header.id &= ~(DEVICE_ERR_MASK);
        }
    } // END FOREVER LOOP
}