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
static xTimerHandle  xGPS_timer;         // handle for the hourly timer
static StaticTimer_t xGPS_timerbuf;      // static buffer to hold timer state


int32_t GPS_task_init(void *profile)
{
    int32_t err;    /* for catching API errors */
    
    eeprom_data.config_settings.gps_config.gps_restRate = 30000;
    eeprom_data.config_settings.gps_config.gps_moveRate = 10000;

    /* initialize the GPS module */
    err = gps_init_i2c(&I2C_GPS) ? ERR_NOT_INITIALIZED : ERR_NONE;

    /* force the GPS to stay awake until configuration is complete */
    gpio_set_pin_level(GPS_EXT_INT, true);

    /* verify/load GPS settings, set up NAV polling, and disable output for now */
    portENTER_CRITICAL();
    if (ERR_NONE == err && GPS_SUCCESS != gps_checkconfig()) {
        err =   gps_reconfig() ||  gps_setrate(10000) ? ERR_NOT_READY : ERR_NONE;  // TODO change to 0
    }
    portEXIT_CRITICAL();

    // TODO what to do if this fails? Should be handled in SW  
    if (err) {
        gpio_toggle_pin_level(LED_RED);
    } else {
        
        /* create a timer with a one hour period for controlling sensors */
        configASSERT(!configUSE_16_BIT_TICKS);
        xGPS_timer = xTimerCreateStatic(   "GPSTimer",                 /* text name of the timer       */
                                            pdMS_TO_TICKS(60000),       /* timer period in ticks (1min) */
                                            pdFALSE,                    /* manual reload after expire   */
                                            (void*)0,                   /* id of expiration counter     */
                                            GPS_movement_cb,            /* timer expiration callback    */
                                            &xGPS_timerbuf);           /* timer data buffer            */
        
        if (NULL == xGPS_timer) { /* if the timer was not created */
            err = DEVICE_ERR_TIMEOUT; // TODO determine proper handling
        }
        
        /* create the task, return ERR_NONE or ERR_NO_MEMORY if the task creation failed */
        xGPS_th = xTaskCreateStatic(GPS_task, "GPS", GPS_STACK_SIZE, (void *)profile, GPS_TASK_PRI, xGPS_stack, &xGPS_taskbuf);
        configASSERT(xGPS_th);
    }
    
    return err;
}

void GPS_task(void *pvParameters)
{
    uint16_t    activehours;        /* number of hours in a day the gps is on   */
    uint16_t    moveminutes;        /* high resolution time(m) allowed per hour */
    uint32_t    samplerate;         /* rate to collect fix and report message   */
    uint32_t    ulNotifyValue;      /* holds the notification bits from the ISR */
    int32_t     err;                /* for catching API errors                  */
    BaseType_t  xResult;            /* holds return value of blocking function  */
    TickType_t  xMaxBlockTime;      /* max time to wait for the task to resume  */
    static GPS_MSG_t gps_msg;	    /* holds the GPS message to store in flash  */

    (void)pvParameters;
    activehours = 0;
    
    for(int i = 0; i < 24; i++) {   /* determine the amount of active hours per day */
        activehours += (eeprom_data.config_settings.gps_config.gps_activeHour >> i) & 1;
    }
    activehours = 24; // TODO remove, testing only
    moveminutes = GPS_MAXMOVE / activehours;   /* determine the high-res time per hour */
    
    /* set the default sample rate */ // TODO: allow flexibility in message rate or fix to sample rate
    samplerate = eeprom_data.config_settings.gps_config.gps_restRate;
    samplerate = 10000;
    portENTER_CRITICAL();
    err = gps_setrate(samplerate) || gps_savecfg(0xFFFF) ? ERR_NOT_READY : ERR_NONE;
    portEXIT_CRITICAL();
    
    // TODO what to do if this fails? Should be handled in SW
    if (err) {
        gpio_toggle_pin_level(LED_RED);
    }
    
    /* update the maximum blocking time to current FIFO full time + <max sensor time> */
    xMaxBlockTime = pdMS_TO_TICKS(samplerate*GPS_LOGSIZE+4000);	// TODO calculate based on registers

    /* initialize the message header */
    gps_msg.header.startSym     = MSG_START_SYM;
    gps_msg.header.id           = DEVICE_ID_GPS;
    gps_msg.header.timestamp    = 0;
    gps_msg.header.msTime       = 0;
    gps_msg.header.size         = 0;
    
    /* clear the GPS FIFO */
    portENTER_CRITICAL();
    gps_readfifo();
    portEXIT_CRITICAL();
    
    /* ensure the TX_RDY interrupt is deactivated */
    gpio_set_pin_level(GPS_TXD, true);
    
    /* enable the data ready interrupt (TxReady) */
    ext_irq_register(GPS_TXD, GPS_isr_dataready);
    
    for (;;) {
        /* put the GPS device to sleep until an interrupting event */
        gpio_set_pin_level(GPS_EXT_INT, false);
        
        /* wait for notification from ISR, returns `pdTRUE` if task, else `pdFALSE` */
        xResult = xTaskNotifyWait( GPS_NOTIFY_NONE, /* bits to clear on entry       */
                                   GPS_NOTIFY_ALL,  /* bits to clear on exit        */
                                   &ulNotifyValue,  /* stores the notification bits */
                                   xMaxBlockTime ); /* max wait time before error   */
                                   
        /* keep the GPS awake until the interrupt is handled */
        gpio_set_pin_level(GPS_EXT_INT, true);
        
        if (pdPASS == xResult) { /* there was an interrupt */
            /* reload the high-res movement minutes if needed */
            if (GPS_NOTIFY_HOUR & ulNotifyValue) {
                moveminutes = GPS_MAXMOVE / activehours;
            }
            
            /* if the ISR indicated that data is ready */
            if (GPS_NOTIFY_TXRDY & ulNotifyValue) {

                /* copy the GPS FIFO over I2C */
                portENTER_CRITICAL();
                err = gps_readfifo() ? ERR_TIMEOUT : ERR_NONE;
                portEXIT_CRITICAL();

                /* and log it, noting communication error if needed */
                GPS_log(&gps_msg, &err, DEVICE_ERR_COMMUNICATIONS);
                
               // gps_nap(samplerate);
            }
            
            /* if motion has been detected by the IMU */
            if (GPS_NOTIFY_MOTION & ulNotifyValue && moveminutes) {
                /* block the state machine from sending another rate change request */
                xEventGroupSetBits(xSYSEVENTS_handle, EVENT_GPS_COOLDOWN);
                
                /* decrement the available high-res minutes and set the rate */
                moveminutes--;
                samplerate = eeprom_data.config_settings.gps_config.gps_moveRate;
                portENTER_CRITICAL();
                err = gps_setrate(samplerate) ? ERR_NO_CHANGE : ERR_NONE;
                portEXIT_CRITICAL();
                
                /* if failure, try again. else start a one minute timer */
                if(err) {
                    xTaskNotify(xGPS_th, GPS_NOTIFY_MOTION, eSetBits);
                } else {
                    xTimerChangePeriod(xGPS_timer, pdMS_TO_TICKS(60000), 0);
                }
            }
            
            /* if the high precision motion timer has expired */
            if (GPS_NOTIFY_REVERT & ulNotifyValue) {
                /* revert the rate back to the resting rate */
                samplerate = eeprom_data.config_settings.gps_config.gps_restRate;
                portENTER_CRITICAL();
                err = gps_setrate(samplerate) ? ERR_NO_CHANGE : ERR_NONE;
                portEXIT_CRITICAL();
                
                /* try again if not successful */
                if (err) {
                    xTaskNotify(xGPS_th, GPS_NOTIFY_REVERT, eSetBits);
                }
            }
        } else { /* the interrupt timed out, figure out why and log */
            /* check how many samples are in the FIFO */
            portENTER_CRITICAL();
            err = gps_checkfifo();
            portEXIT_CRITICAL();

            /* log the error based on FIFO state */
            if (GPS_FIFOSIZE < err) {
                err = ERR_OVERFLOW;
                GPS_log(&gps_msg, &err, DEVICE_ERR_OVERFLOW | DEVICE_ERR_TIMEOUT);
                portENTER_CRITICAL();
                gps_readfifo();
                portEXIT_CRITICAL();
            } else {
                err = ERR_TIMEOUT;
                GPS_log(&gps_msg, &err, DEVICE_ERR_TIMEOUT);
            } // TODO expand to handle unchanged FIFO (maybe readout and reset)
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

void GPS_movement_cb(TimerHandle_t xTimer)
{
//     /* tell the GPS to revert to resting rate, try again if busy */
//     if (xTaskNotify(xGPS_th, GPS_NOTIFY_NONE, eSetValueWithoutOverwrite)) {
//         xTimerChangePeriod(xTimer, pdMS_TO_TICKS(1000), 0);
//     }
//     xEventGroupClearBits(xSYSEVENTS_handle, EVENT_GPS_COOLDOWN);
}

void GPS_log(GPS_MSG_t *msg, int32_t *err, const DEVICE_ERR_CODES_t ERR_CODES)
{
    uint16_t    logcount;           /* how many log entries were parsed */
    uint16_t    logsize;            /* size in bytes of the log message */

    /* set the timestamp and any error flags to the log message */
    timestamp_FillHeader(&msg->header);

    if (*err < 0) {
        /* if an error occurred with the FIFO, log the error */
        msg->header.id  |= ERR_CODES;
        msg->header.size = 0;
        logsize = sizeof(DATA_HEADER_t);
    } else { 
        /* otherwise, extract the GPS data and log it */
        logcount = gps_parsefifo(msg->log, GPS_LOGSIZE);
        msg->header.size = logcount * sizeof(gps_log_t);
        logsize = sizeof(DATA_HEADER_t) + msg->header.size;
    }

    /* write the message to flash */
    *err = ctrlLog_write((uint8_t*)msg, logsize);
    if (*err < 0) { // TODO handle incomplete logs elsewhere
        gpio_toggle_pin_level(LED_RED);
    }
}