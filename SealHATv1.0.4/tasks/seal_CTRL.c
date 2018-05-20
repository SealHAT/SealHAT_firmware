/*
 * seal_MSG.c
 *
 * Created: 30-Apr-18 22:53:41
 *  Author: Ethan
 */

#include "seal_CTRL.h"
#include "seal_USB.h"
#include "sealPrint.h"
#include "storage\flash_io.h"

TaskHandle_t         xCTRL_th;     // Message accumulator for USB/MEM
EventGroupHandle_t   xCTRL_eg;     // IMU event group
SemaphoreHandle_t    DATA_mutex;   // mutex to control access to USB terminal
StreamBufferHandle_t xDATA_sb;     // stream buffer for getting data into FLASH or USB

static FLASH_DESCRIPTOR seal_flash_descriptor; /* Declare flash descriptor. */

void vbus_detection_cb(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult;

    if( gpio_get_pin_level(VBUS_DETECT) ) {
        usb_start();
        xResult = xEventGroupSetBitsFromISR(xCTRL_eg, EVENT_VBUS, &xHigherPriorityTaskWoken);
    }
    else {
        usb_stop();
        xResult = xEventGroupClearBitsFromISR(xCTRL_eg, EVENT_VBUS);
    }

    if(xResult != pdFAIL) {
        /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch should be requested. */
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

void timestamp_FillHeader(DATA_HEADER_t* header)
{
    // get the time in seconds since (the custom set) epoch
    header->timestamp = _calendar_get_counter(&RTC_CALENDAR.device);

    // force a sync on the counter value and get the sub second value (2048 per second, or about .5 mSec)
    hri_tc_set_CTRLB_CMD_bf(TC4, TC_CTRLBSET_CMD_READSYNC_Val);
    header->msTime = hri_tccount16_get_COUNT_reg(TC4, 0xFFFF);
}

int32_t ctrlLog_write(uint8_t* buff, const uint32_t LEN)
{
    uint32_t err;

    if(xSemaphoreTake(DATA_mutex, ~0)) {

        // bail early if there isn't enough space
        portENTER_CRITICAL();
        if(xStreamBufferSpacesAvailable(xDATA_sb) >= LEN) {
            err = xStreamBufferSend(xDATA_sb, buff, LEN, 0);
        }
        else {
            err = ERR_NO_RESOURCE;
            gpio_set_pin_level(LED_RED, false);
        }
        portEXIT_CRITICAL();

        xSemaphoreGive(DATA_mutex);
    }
    else {
        err = ERR_FAILURE;
        gpio_set_pin_level(LED_RED, false);
    }

    return err;
}

int32_t ctrlLog_writeISR(uint8_t* buff, const uint32_t LEN)
{
    uint32_t err;

    if(xSemaphoreTake(DATA_mutex, ~0)) {

        // bail early if there isn't enough space
        portENTER_CRITICAL();
        if(xStreamBufferSpacesAvailable(xDATA_sb) >= LEN) {
            err = xStreamBufferSendFromISR(xDATA_sb, buff, LEN, 0);
        }
        else {
            err = ERR_NO_RESOURCE;
        }
        portEXIT_CRITICAL();

        xSemaphoreGive(DATA_mutex);
    }
    else {
        err = ERR_FAILURE;
    }

    return err;
}

int32_t CTRL_task_init(void)
{
    struct calendar_date date;
    struct calendar_time time;
    int32_t err = ERR_NONE;
    static uint8_t readBuf[64]; // TODO: delete this array after configuration struct has been added to code base
    int retVal; // TODO: do something with this value or remove it

    // create 24-bit system event group
    xCTRL_eg = xEventGroupCreate();
    if(xCTRL_eg == NULL) {
        return ERR_NO_MEMORY;
    }

    calendar_enable(&RTC_CALENDAR);

    date.year  = 2018;
    date.month = 5;
    date.day   = 4;

    time.hour = 15;
    time.min  = 33;
    time.sec  = 0;

    err = calendar_set_date(&RTC_CALENDAR, &date);
    if(err != ERR_NONE) { return err; }

    err = calendar_set_time(&RTC_CALENDAR, &time);
    if(err != ERR_NONE) { return err; }

    // initialize (clear all) event group and check current VBUS level
    xEventGroupClearBits(xCTRL_eg, EVENT_MASK_ALL);
    if(gpio_get_pin_level(VBUS_DETECT)) {
        usb_start();
        xEventGroupSetBits(xCTRL_eg, EVENT_VBUS);
    }

    DATA_mutex = xSemaphoreCreateMutex();
    if (DATA_mutex == NULL) {
        return ERR_NO_MEMORY;
    }

    xDATA_sb = xStreamBufferCreate(DATA_QUEUE_LENGTH, 64);
    if(xDATA_sb == NULL) {
        return ERR_NO_MEMORY;
    }

    /* TODO: This is a stand-in read for reading the config struct from the EEPROM. Once the config struct
     * is set up, this call should be updated to read data into said struct on device startup. */
    retVal = flash_read(&FLASH_NVM, CONFIG_BLOCK_BASE_ADDR, readBuf, NVMCTRL_PAGE_SIZE);

    return ( xTaskCreate(CTRL_task, "MSG", MSG_STACK_SIZE, NULL, MSG_TASK_PRI, &xCTRL_th) == pdPASS ? ERR_NONE : ERR_NO_MEMORY);
}

void CTRL_task(void* pvParameters)
{
    static uint8_t endptBuf[PAGE_SIZE_EXTRA];       // hold the received messages
    (void)pvParameters;

    /* Initialize flash device(s). */
    flash_io_init(&seal_flash_descriptor, PAGE_SIZE_LESS);

    // register VBUS detection interrupt
    ext_irq_register(VBUS_DETECT, vbus_detection_cb);

    // set the stream buffer trigger level for USB
    xStreamBufferSetTriggerLevel(xDATA_sb, PAGE_SIZE_LESS);

    /* Receive and write data forever. */
    for(;;)
    {
        /* Receive a page worth of data. */
        xStreamBufferReceive(xDATA_sb, endptBuf, PAGE_SIZE_LESS, portMAX_DELAY);

        /* Write data to external flash device. */
        flash_io_write(&seal_flash_descriptor, endptBuf, PAGE_SIZE_LESS);
    }
}