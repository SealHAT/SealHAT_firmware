/*
 * seal_GPS.c
 *
 * Created: 5/11/2018 2:36:10 PM
 *  Author: Anthony Koutroulis
 */

 #include "seal_GPS.h"
 #include "seal_DATA.h"

TaskHandle_t xGPS_th;                    // GPS task handle
StaticTask_t xGPS_taskbuf;               // task buffer for the GPS task
StackType_t  xGPS_stack[GPS_STACK_SIZE]; // static stack allocation for GPS task

int32_t GPS_task_init(void *profile)
{
    /* create the task, return ERR_NONE or ERR_NO_MEMORY if the task creation failed */
    xGPS_th = xTaskCreateStatic(GPS_task, "GPS", GPS_STACK_SIZE, (void *)profile, GPS_TASK_PRI, xGPS_stack, &xGPS_taskbuf);
    configASSERT(xGPS_th);

    return ERR_NONE;
}

void GPS_task(void *pvParameters)
{
    static GPS_MSG_t gps_msg;	        /* holds the GPS message to store in flash  */
    int32_t     err     = ERR_NONE;     /* for catching API errors                  */
    BaseType_t  xResult;                /* holds return value of blocking function  */
    TickType_t  xMaxBlockTime;          /* max time to wait for the task to resume  */
    uint32_t    ulNotifyValue;          /* holds the notification bits from the ISR */
    uint16_t    logcount;               /* how many log entries were parsed         */
    
    /* initialize the GPS module */
	portENTER_CRITICAL();
	err = gps_init_i2c(&I2C_GPS) || gps_selftest() ? ERR_NOT_INITIALIZED : ERR_NONE;
	gpio_set_pin_level(GPS_TXD, true);
	portEXIT_CRITICAL();
    // TODO what to do if this fails? Will be handled in SW
    if (err) {
		gpio_toggle_pin_level(LED_RED);
	}

    /* update the maximum blocking time to current FIFO full time + <max sensor time> */
    xMaxBlockTime = pdMS_TO_TICKS(26000);	// TODO calculate based on registers

    /* initialize the message header */
    gps_msg.header.startSym     = MSG_START_SYM;
    gps_msg.header.id           = DEVICE_ID_GPS;
    gps_msg.header.timestamp    = 0;
    gps_msg.header.msTime       = 0;
    gps_msg.header.size         = sizeof(gps_log_t)*GPS_LOGSIZE;

    /* enable the data ready interrupt (TxReady) */
    ext_irq_register(GPS_TXD, GPS_isr_dataready);

    for (;;) {
        /* wait for notification from ISR, returns `pdTRUE` if task, else `pdFALSE` */
        xResult = xTaskNotifyWait( GPS_NOTIFY_NONE, /* bits to clear on entry       */
                                   GPS_NOTIFY_ALL,  /* bits to clear on exit        */
                                   &ulNotifyValue,   /* stores the notification bits */
                                   xMaxBlockTime ); /* max wait time before error   */

        if (pdPASS == xResult) {
            /* if the ISR indicated that data is ready */
            if ( GPS_NOTIFY_TXRDY & ulNotifyValue) {
                /* copy the GPS FIFO over I2C */
                portENTER_CRITICAL();
                err = gps_readfifo() ? ERR_TIMEOUT : ERR_NONE;
                portEXIT_CRITICAL();

                /* set the timestamp and any error flags to the log message */
                timestamp_FillHeader(&gps_msg.header);
                if (ERR_NONE != err) { /* log error */
                    gps_msg.header.id |= DEVICE_ERR_COMMUNICATIONS;
                    gps_msg.header.size = 0;
                    err = ctrlLog_write((uint8_t*)&gps_msg, sizeof(DATA_HEADER_t));
                    if (err < 0) {
						gpio_toggle_pin_level(LED_RED);
					}
                } else { /* no error */
                    logcount = gps_parsefifo(GPS_FIFO, gps_msg.log, GPS_LOGSIZE);
                    // TODO - snoop the logs and do something if consistently invalid
                    gps_msg.header.size = logcount * sizeof(gps_log_t);
                    err = ctrlLog_write((uint8_t*)&gps_msg, sizeof(DATA_HEADER_t) + gps_msg.header.size);
                    if (err < 0) {
						gpio_toggle_pin_level(LED_RED);
					}
                }
            }
        } else { /* the interrupt timed out */
            // TODO - implement FIFO ready count (in gps.c) and do something with that
			err = gps_checkfifo();
			if (err < 0) {
				gpio_toggle_pin_level(LED_RED);
			} else {
				gpio_toggle_pin_level(LED_RED);
			}
        }
    } // END FOREVER LOOP
}

void GPS_isr_dataready(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // will be set to true by notify if we are awakening a higher priority task

    /* Notify the GPS task that the FIFO has enough data to trigger the TxReady interrupt */
    xTaskNotifyFromISR(xGPS_th, GPS_NOTIFY_TXRDY, eSetBits, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
    should be performed to ensure the interrupt returns directly to the highest
    priority task.*/
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

