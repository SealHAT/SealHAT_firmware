/*
 * seal_MSG.c
 *
 * Created: 30-Apr-18 22:53:41
 *  Author: Ethan
 */

#include "seal_CTRL.h"

EEPROM_STORAGE_t eeprom_data;                       //struct containing sensor and SealHAT configurations

TaskHandle_t         xCTRL_th;                      // Message accumulator for USB/MEM
static StaticTask_t  xCTRL_taskbuf;                 // task buffer for the CTRL task
static StackType_t   xCTRL_stack[CTRL_STACK_SIZE];  // static stack allocation for CTRL task
static xTimerHandle  xCTRL_timer;                   // handle for the hourly timer
static StaticTimer_t xCTRL_timerbuf;                // static buffer to hold timer state

EventGroupHandle_t        xSYSEVENTS_handle;        // system events event group
static StaticEventGroup_t xSYSEVENTS_eventgroup;    // static memory for the event group


void vbus_detection_cb(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult;

    if( gpio_get_pin_level(VBUS_DETECT) ) {
        usb_start();
        xResult = xEventGroupSetBitsFromISR(xSYSEVENTS_handle, EVENT_VBUS, &xHigherPriorityTaskWoken);
    }
    else {
        usb_stop();
        xResult = xEventGroupClearBitsFromISR(xSYSEVENTS_handle, EVENT_VBUS);
    }

    if(xResult != pdFAIL) {
        /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch should be requested. */
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

void vHourlyTimerCallback( TimerHandle_t xTimer )
{
    configASSERT(xTimer);
    // TODO add task checking code here
    xTimerChangePeriod(xTimer, pdMS_TO_TICKS(3600000), 0);
}

int32_t CTRL_task_init(void)
{
    int32_t err = ERR_NONE;

    // create 24-bit system event group for system alerts
    xSYSEVENTS_handle = xEventGroupCreateStatic(&xSYSEVENTS_eventgroup);
    configASSERT(xSYSEVENTS_handle);

    /* Read stored device settings from EEPROM and make them accessible to all devices. */
    err = eeprom_read_configs(&eeprom_data);

    /* initialize (clear all) event group and check current VBUS level */
    xEventGroupClearBits(xSYSEVENTS_handle, EVENT_MASK_ALL);
    if(gpio_get_pin_level(VBUS_DETECT)) {
        usb_start();
        xEventGroupSetBits(xSYSEVENTS_handle, EVENT_VBUS);
    }
    
    /* create a timer with a one hour period for controlling sensors */
    configASSERT(!configUSE_16_BIT_TICKS);
    xCTRL_timer = xTimerCreateStatic(   "HourTimer",                /* text name of the timer       */
                                        pdMS_TO_TICKS(HOUR_MS),     /* timer period in ticks        */
                                        pdFALSE,                    /* manual reload after expire   */
                                        (void*)0,                   /* id of expiration counter     */
                                        vHourlyTimerCallback,       /* timer expiration callback    */
                                        &xCTRL_timerbuf);           /* timer data buffer            */
    if (NULL == xCTRL_timer) { /* if the timer was not created */
        err = DEVICE_ERR_TIMEOUT; // TODO determine proper handling
    } /* timer will not start until an update time event to prevent scheduling before RTC is set */    

    xCTRL_th = xTaskCreateStatic(CTRL_task, "CTRL", CTRL_STACK_SIZE, NULL, CTRL_TASK_PRI, xCTRL_stack, &xCTRL_taskbuf);
    configASSERT(xCTRL_th);

    return err;
}

void CTRL_task(void* pvParameters)
{
    int32_t err;
    (void)pvParameters;

    // register VBUS detection interrupt
    ext_irq_register(VBUS_DETECT, vbus_detection_cb);

    /* monitor system events and time to control states */
    for(;;) {
                
        /* check if the system time has changed */
        if ( 0 != (xEventGroupGetBits(xSYSEVENTS_handle) & EVENT_TIME_CHANGE) ) {
            xEventGroupClearBits(xSYSEVENTS_handle, EVENT_TIME_CHANGE);
            CTRL_timer_update(xCTRL_timer);
        }
        os_sleep(pdMS_TO_TICKS(500));
    }
}

void CTRL_timer_update(TimerHandle_t xTimer)
{
    uint32_t msoffset;
    struct calendar_date_time datetime;
    
    /* set the timer to expire at the top of the next hour */
    calendar_get_date_time(&RTC_CALENDAR, &datetime);
    msoffset    = 60000*(59 - datetime.time.min) + 1000*(60 - (datetime.time.sec % 60));
    xTimerChangePeriod(xTimer, pdMS_TO_TICKS(msoffset), 0);
}
