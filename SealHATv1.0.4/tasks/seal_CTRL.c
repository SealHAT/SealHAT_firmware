/*
 * seal_MSG.c
 *
 * Created: 30-Apr-18 22:53:41
 *  Author: Ethan
 */

#include "seal_CTRL.h"

EEPROM_STORAGE_t eeprom_data;                       //struct containing sensor and SealHAT configurations

TaskHandle_t        xCTRL_th;                       // Message accumulator for USB/MEM
static StaticTask_t xCTRL_taskbuf;                  // task buffer for the CTRL task
static StackType_t  xCTRL_stack[CTRL_STACK_SIZE];   // static stack allocation for CTRL task

EventGroupHandle_t        xSYSEVENTS_handle;        // IMU event group
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

int32_t CTRL_task_init(void)
{
    int32_t err = ERR_NONE;

    // create 24-bit system event group for system alerts
    xSYSEVENTS_handle = xEventGroupCreateStatic(&xSYSEVENTS_eventgroup);
    configASSERT(xSYSEVENTS_handle);

    /* Read stored device settings from EEPROM and make them accessible to all devices. */
    err = eeprom_read_configs(&eeprom_data);

    /* initialize (clear all) event group and check current VBUS level*/
    xEventGroupClearBits(xSYSEVENTS_handle, EVENT_MASK_ALL);
    if(gpio_get_pin_level(VBUS_DETECT)) {
        usb_start();
        xEventGroupSetBits(xSYSEVENTS_handle, EVENT_VBUS);
    }

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

    /* Receive and write data forever. */
    for(;;) {
        // ADD CONTROL CODE HERE
        os_sleep(pdMS_TO_TICKS(500));
    }
}
