/*
 * seal_ENV.c
 *
 * Created: 30-Apr-18 22:53:14
 *  Author: Ethan
 */

#include "seal_ENV.h"

#define TEMP_READ_TIME      (pdMS_TO_TICKS(8)) // time to block between reading start and get in ms

TaskHandle_t      xENV_th;      // environmental sensors task (light and temp)

int32_t ENV_task_init(uint32_t period)
{
    return (xTaskCreate(ENV_task, "ENV", ENV_STACK_SIZE, (void*)period, ENV_TASK_PRI, &xENV_th) == pdPASS ? ERR_NONE : ERR_NO_MEMORY);
}

void ENV_task(void* pvParameters)
{
    //    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    int32_t     err     = ERR_NONE;     // for catching API errors
    TickType_t  xPeriod;                // the period of the sampling in seconds
    ENV_MSG_t   msg;                    // reads the data
    TickType_t  xLastWakeTime;          // last wake time variable for timing
    (void)pvParameters;

    // initialize the temperature sensor
    si705x_init(&I2C_ENV);

    // initialize the light sensor
    max44009_init(&I2C_ENV, LIGHT_ADD_GND);

    //    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

    // set the header data
    msg.header.srtSym    = 0xDEAD;
    msg.header.size      = 4;   // four bytes of data in an env packet

    // Initialize the xLastWakeTime variable with the current time.
    xPeriod       = pdMS_TO_TICKS((uint32_t)pvParameters * 1000);
    xLastWakeTime = xTaskGetTickCount();

    for(;;) {

        // initialize the start time, or re-init if the task was suspended
        if((xLastWakeTime + xPeriod) < xTaskGetTickCount()) {
            xLastWakeTime = xTaskGetTickCount();
        }
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        // reset the message header
        msg.header.id        = DEV_ENV;
        msg.header.timestamp = _calendar_get_counter(&RTC_CALENDAR.device);
        hri_tc_set_CTRLB_CMD_bf(TC4, TC_CTRLBSET_CMD_READSYNC_Val);
        msg.header.msTime    = hri_tccount16_get_COUNT_reg(TC4, 0xFFFF);

        // start an asynchronous temperature reading
        portENTER_CRITICAL();
        err = si705x_measure_asyncStart();
        portEXIT_CRITICAL();

        //  read the light level
        portENTER_CRITICAL();
        msg.light = max44009_read_uint16();
        portEXIT_CRITICAL();

        //        uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

        // wait for the temperature sensor to finish
        os_sleep(TEMP_READ_TIME);

        // get temp
        portENTER_CRITICAL();
        msg.temp  = si705x_measure_asyncGet(&err);
        portEXIT_CRITICAL();
        if(err < 0) {
            msg.header.id |= ERROR_TEMP;
        }

        byteQ_write((uint8_t*)&msg, sizeof(ENV_MSG_t));
    }
} 