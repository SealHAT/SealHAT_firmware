/*
 * seal_MSG.c
 *
 * Created: 30-Apr-18 22:53:41
 *  Author: Ethan
 */

#include "seal_CTRL.h"

TaskHandle_t         xCTRL_th;     // Message accumulator for USB/MEM
EventGroupHandle_t   xCTRL_eg;     // IMU event group
SemaphoreHandle_t    DATA_mutex;   // mutex to control access to USB terminal
StreamBufferHandle_t xDATA_sb;     // stream buffer for getting data into FLASH or USB

    
static FLASH_DESCRIPTOR flash_descriptor; /* Declare flash descriptor. */
const uint8_t WELCOME[] = "Welcome\n";
const uint8_t NEW_LINE[] = "\n";

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
        }
        portEXIT_CRITICAL();

        xSemaphoreGive(DATA_mutex);
    }
    else {
        err = ERR_FAILURE;
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

int32_t CTRL_task_init(uint32_t qLength)
{
    struct calendar_date date;
    struct calendar_time time;
    int32_t err = ERR_NONE;

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

    xDATA_sb = xStreamBufferCreate(qLength, 64);
    if(xDATA_sb == NULL) {
        return ERR_NO_MEMORY;
    }
    
    /* Flash storage initialization. */
    //flash_io_init(&flash_descriptor, PAGE_SIZE_LESS);

    return ( xTaskCreate(CTRL_task, "MSG", MSG_STACK_SIZE, NULL, MSG_TASK_PRI, &xCTRL_th) == pdPASS ? ERR_NONE : ERR_NO_MEMORY);
}

void CTRL_task(void* pvParameters)
{
    int retVal;
    static const uint8_t BUFF_SIZE = 64;
    static uint8_t endptBuf[PAGE_SIZE_EXTRA];       // hold the received messages
    (void)pvParameters;
    
    /* Initialize flash device(s). */
    flash_io_init(&flash_descriptor, PAGE_SIZE_LESS);

    // register VBUS detection interrupt
    ext_irq_register(VBUS_DETECT, vbus_detection_cb);

    // set the stream buffer trigger level for USB
    xStreamBufferSetTriggerLevel(xDATA_sb, BUFF_SIZE);
    
    xStreamBufferReceive(xDATA_sb, endptBuf, BUFF_SIZE, portMAX_DELAY);
    
    do { /* NOTHING */ } while (!usb_dtr());
        
    // print welcome
    if(usb_state() == USB_Configured && usb_dtr()) {
        usb_write(WELCOME, 8);
    }
    
    // print old buffer data
    do {
        retVal = usb_write(endptBuf, BUFF_SIZE);
    } while((usb_dtr() == false) || (retVal != USB_OK));
    
    // print newline character to console
    do {
        retVal = usb_write(NEW_LINE, 1);
    } while((usb_dtr() == false) || (retVal != USB_OK));
        
    // write data once buffer is full
    flash_io_write(&flash_descriptor, endptBuf, BUFF_SIZE);
        
    // flush the flash buffer and reset the address pointer
    flash_io_flush(&flash_descriptor);
    flash_io_reset_addr();
    
    delay_ms(10);

    flash_io_read(&flash_descriptor, endptBuf, PAGE_SIZE_LESS);
    
    if(usb_isInBusy() == true) { /* Wait */ }
        
    // print new buffer data
    do{
        retVal = usb_write(endptBuf, BUFF_SIZE);
    } while((usb_dtr() == false) || (retVal != USB_OK));

    for(;;) 
    {
        gpio_toggle_pin_level(LED_GREEN);
        delay_ms(500);
    }
}