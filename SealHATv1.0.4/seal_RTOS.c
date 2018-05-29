/*
 * seal_RTOS.c
 *
 * Created: 30-Apr-18 23:23:33
 *  Author: Ethan
 */
#include "seal_RTOS.h"

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
    TaskHandle_t            xHandle;
    TaskStatus_t            xTaskDetails;
    STACK_OVERFLOW_PACKET_t msg;
    (void)xTask;

    /* Obtain the handle of a task from its name. */
    xHandle = xTaskGetHandle((char*)pcTaskName);

    /* Check the handle is not NULL. */
    configASSERT(xHandle);

    /* Use the handle to obtain further information about the task. */
    vTaskGetInfo(xHandle,           // task handle to get info about
                 &xTaskDetails,    // Return structure
                 pdFALSE,          // Don't get high water mark since we are obviously overflown
                 eInvalid );       // Don't get task state since it has died and we are about to revive it

    msg.header.startSym = MSG_START_SYM;
    msg.header.id       = DEVICE_ID_SYSTEM | DEVICE_ERR_OVERFLOW;
    timestamp_FillHeader(&msg.header);
    msg.header.size     = snprintf((char*)msg.buff, STACK_OVERFLOW_DATA_SIZE, "OVF,%s", pcTaskName);
    msg.header.size     = (msg.header.size > STACK_OVERFLOW_DATA_SIZE ? STACK_OVERFLOW_DATA_SIZE : msg.header.size);

    // TODO: write the error message to the EEPROM so that it can be logged to flash on reboot
    //ctrlLog_write(&msg, sizeof(STACK_OVERFLOW_PACKET_t));

    // TODO: either use this reset function, let watchdog trigger, or both.
    _reset_mcu();
}

int32_t checkResetReason(void) {
    enum reset_reason cause;
    cause = _get_reset_reason();

//     switch(cause) {
//         case RESET_REASON_WDT    :
//                                    break;
//         case RESET_REASON_SYST   :
//                                    break;
//         case RESET_REASON_POR    :
//                                    break;
//         case RESET_REASON_BOD12  :
//                                    break;
//         case RESET_REASON_BOD33  :
//                                    break;
//         case RESET_REASON_EXT    :
//                                    break;
//         case RESET_REASON_WDT    :
//                                    break;
//         case RESET_REASON_SYST   :
//                                    break;
//         case RESET_REASON_BACKUP :
//                                    break;
//         default:
//     };

    return (int32_t)cause;
}

void timestamp_FillHeader(DATA_HEADER_t* header)
{
    // get the time in seconds since (the custom set) epoch
    header->timestamp = _calendar_get_counter(&RTC_CALENDAR.device);

    // force a sync on the counter value and get the sub second value (2048 per second, or about .5 mSec)
    hri_tc_set_CTRLB_CMD_bf(TC4, TC_CTRLBSET_CMD_READSYNC_Val);
    header->msTime = hri_tccount16_get_COUNT_reg(TC4, 0xFFFF);
}

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

/*************************************************************
 * FUNCTION: eeprom_save_configs()
 * -----------------------------------------------------------
 * This function writes the SealHAT device's sensor and
 * configuration data out to the chip's onboard EEPROM.
 *
 * Parameters:
 *      config_settings : Pointer to struct of config settings.
 *
 * Returns:
 *      The error value of the flash_write operation.
 *************************************************************/
uint32_t eeprom_save_configs(EEPROM_STORAGE_t *config_settings)
{
    uint32_t retVal;

    /* Flash must be erased before a new value may be written to it. */
    retVal = flash_erase(&FLASH_NVM, CONFIG_BLOCK_BASE_ADDR, sizeof(EEPROM_STORAGE_t));

    /* If the erase operation succeeded, write the new data to the EEPROM.
     * Otherwise, return the error value. */
    if(retVal == ERR_NONE)
    {
        /* Cast pointer to config struct to a uint8 pointer. */
        retVal = flash_write(&FLASH_NVM, CONFIG_BLOCK_BASE_ADDR, (uint8_t *) config_settings, sizeof(EEPROM_STORAGE_t));
    }

    return (retVal);
}

/*************************************************************
 * FUNCTION: eeprom_read_configs()
 * -----------------------------------------------------------
 * This function reads the SealHAT device's sensor and
 * configuration settings from the onboard EEPROM.
 *
 * Parameters:
 *      config_settings : Pointer to struct of config settings.
 *
 * Returns:
 *      The error value of the flash_read operation.
 *************************************************************/
uint32_t eeprom_read_configs(EEPROM_STORAGE_t *config_settings)
{
    /* Cast pointer to config struct to a uint8 pointer. */
    return(flash_read(&FLASH_NVM, CONFIG_BLOCK_BASE_ADDR, (uint8_t *) config_settings, sizeof(EEPROM_STORAGE_t)));
}