/*
 * seal_GPS.c
 *
 * Created: 5/11/2018 2:36:10 PM
 *  Author: Anthony Koutroulis
 */ 

 #include "seal_GPS.h"

TaskHandle_t    xGPS_th;    /* GPS task handle */

int32_t GPS_task_init(void *profile)
{
    /* create the task, return ERR_NONE or ERR_NO_MEMORY if the task creation failed */
    return ( xTaskCreate(GPS_task, "GPS", GPS_STACK_SIZE, (void *)profile, GPS_TASK_PRI, &xGPS_th) == pdPASS ? ERR_NONE : ERR_NO_MEMORY);
}

void GPS_task(void *pvParameters)
{
    int32_t     err     = ERR_NONE;     /* for catching API errors                  */
    GPS_MSG_t   msg;                    /* holds the gps message to store in flash  */
    
    /* initialize the GPS module */
    gps_init_i2c(&I2C_IMU);

    /* initialize the message header */
    msg.header.srtSym       = MSG_START_SYM;
    msg.header.id           = DEV_GPS;
    msg.header.timestamp    = 0;
    msg.header.msTime       = 0;
    msg.header.size         = sizeof(location_t)*GPS_FIFO_SIZE;
    
    /* enable the data ready interrupt (TxReady) */
    ext_irq_register(GPS_TXD, GPS_isr_dataready);

    
    for (;;) {
        
    }
}

void GPS_isr_dataready(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // will be set to true by notify if we are awakening a higher priority task

    /* Notify the GPS task that the FIFO has enough data to trigger the TxReady interrupt */
    xTaskNotifyFromISR(xGPS_th, GPS_TXRDY, eSetBits, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task.*/
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

