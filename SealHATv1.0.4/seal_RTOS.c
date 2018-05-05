/*
 * seal_RTOS.c
 *
 * Created: 30-Apr-18 23:23:33
 *  Author: Ethan
 */
#include "seal_RTOS.h"
#include "seal_UTIL.h"

void vApplicationIdleHook(void)
{
    sleep(PM_SLEEPCFG_SLEEPMODE_STANDBY_Val);
}

void vApplicationTickHook(void)
{
    gpio_toggle_pin_level(MOD2);
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
    TaskHandle_t xHandle;
    TaskStatus_t xTaskDetails;
    (void)xTask;

    /* Obtain the handle of a task from its name. */
    xHandle = xTaskGetHandle((char*)pcTaskName);

    /* Check the handle is not NULL. */
    configASSERT(xHandle);

    /* Use the handle to obtain further information about the task. */
    vTaskGetInfo( /* The handle of the task being queried. */
                  xHandle,
                  /* The TaskStatus_t structure to complete with information
                  on xTask. */
                  &xTaskDetails,
                  /* Include the stack high water mark value in the
                  TaskStatus_t structure. */
                  pdTRUE,
                  /* Include the task state in the TaskStatus_t structure. */
                  eInvalid );

    while(1) {;}
}